#include "stdafx.h"
#include "DotDetectorMatrix.h"
#include "DotDetectorLayer.h"

using namespace wdd;

arma::cube DotDetectorM::videoBuffer;
unsigned int DotDetectorM::positionInBuffer;
bool DotDetectorM::initializing;
unsigned int DotDetectorM::resolutionX, DotDetectorM::resolutionY, DotDetectorM::framesInBuffer;

void DotDetectorM::init(unsigned int _resX, unsigned int _resY, unsigned int _framesInBuffer)
{
	videoBuffer = arma::cube(_resX, _resY, _framesInBuffer);
	positionInBuffer = 0;
	initializing = true;
	resolutionX = _resX;
	resolutionY = _resY;
	framesInBuffer = _framesInBuffer;
}




void DotDetectorM::addNewFrame(cv::Mat *_newFrame) {
	// convert cv matrix to armadillo matrix
	double *new_data = new double[_newFrame->rows*_newFrame->cols];
	double *p = new_data;
	for (int i = 0; i < _newFrame->rows; i++){
		memcpy(p, _newFrame->ptr(i), _newFrame->cols*sizeof(double));
		p += _newFrame->cols;
	}
	// put the frame into the buffer
	videoBuffer.slice(positionInBuffer) = arma::mat(new_data, _newFrame->rows, _newFrame->cols);

	// increase position in buffer
	++positionInBuffer;
	if (positionInBuffer > framesInBuffer) {
		positionInBuffer = positionInBuffer % framesInBuffer;
		initializing = false;
	}
}


void DotDetectorM::detectDots() {
	// create projection matrix, that is the matrix with cos and sin values
	arma::mat cosProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
	arma::mat sinProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
	for (unsigned int i(0); i < WDD_FBUFFER_SIZE; ++i) {
		for (unsigned int j(0); j < WDD_FREQ_NUMBER; ++j) {
			cosProjection.at(i, j) = DotDetectorLayer::SAMPLES[i].cosines[j];
			sinProjection.at(i, j) = DotDetectorLayer::SAMPLES[i].sines[j];
		}
	}

	// perform projection for the whole buffer
	arma::mat potentials(resolutionX, resolutionY);
	for (unsigned int i(0); i < resolutionX; ++i) {
		for (unsigned int j(0); j < resolutionY; ++j) {
			arma::colvec timeSlice = videoBuffer.tube(i, j);
			// this is the projection on sine and cosine
			arma::rowvec projectionsSin = timeSlice.t() * sinProjection;
			arma::rowvec projectionsCos = timeSlice.t() * cosProjection;
			uint16_t id = j + i*resolutionY;
			executeDetection(projectionsSin, projectionsCos, id);
		}
	}

}

inline void DotDetectorM::executeDetection(arma::rowvec &_projectedSin, arma::rowvec &_projectedCos, uint16_t _id)
{
	// score_i = sinSum_i^2 + cosSum_i^2
	arma::vec scoresForFrequencies = _projectedSin % _projectedSin + _projectedCos % _projectedCos;

	double buff;
	buff = scoresForFrequencies[0] + scoresForFrequencies[1] + scoresForFrequencies[2];
	for (int i = 3; i < WDD_FREQ_NUMBER; i++){
		if (buff>0.3 * WDD_FBUFFER_SIZE*WDD_FBUFFER_SIZE){
			DotDetectorLayer::DD_SIGNALS_NUMBER++;
			DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
			return;
		}
		buff += -scoresForFrequencies[i - 3] + scoresForFrequencies[i];
	}
	if (buff>0.3 * WDD_FBUFFER_SIZE*WDD_FBUFFER_SIZE){
		DotDetectorLayer::DD_SIGNALS_NUMBER++;
		DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
		return;
	}
}