#include "DotDetectorMatrix.h"
#include "Config.h"
#include "DotDetectorLayer.h"

using namespace wdd;

cv::Mat DotDetectorM::videoBuffer;
//arma::mat DotDetectorM::videoBuffer;
unsigned int DotDetectorM::positionInBuffer;
bool DotDetectorM::initializing;
unsigned int DotDetectorM::resolutionX, DotDetectorM::resolutionY, DotDetectorM::framesInBuffer;

void DotDetectorM::init(unsigned int _resX, unsigned int _resY, unsigned int _framesInBuffer)
{
    std::cout << "Initializing dot detector" << std::endl;
    //int dimensions[3] = { _resX, _resY, _framesInBuffer };
    videoBuffer = cv::Mat(_framesInBuffer, _resX * _resY, CV_32F);
    //videoBuffer = arma::cube(_resX, _resY, _framesInBuffer);
    //videoBuffer = arma::mat(_resX * _resY, _framesInBuffer);
    positionInBuffer = 0;
    initializing = true;
    resolutionX = _resX;
    resolutionY = _resY;
    framesInBuffer = _framesInBuffer;
    std::cout << "Initialization complete" << std::endl;
}

void DotDetectorM::addNewFrame(cv::Mat* _newFrame)
{
    // convert cv matrix to armadillo matrix
    //arma::mat newFrame(reinterpret_cast<double*>(_newFrame->data), _newFrame->rows, _newFrame->cols);
    //newFrame.reshape(1, resolutionX*resolutionY);

    //double *new_data = new double[_newFrame->rows*_newFrame->cols];
    //double *p = new_data;
    //for (int i = 0; i < _newFrame->rows; i++){
    //	memcpy(p, _newFrame->ptr(i), _newFrame->cols*sizeof(double));
    //	p += _newFrame->cols;
    //}
    // put the frame into the buffer
    //std::cout << "Adding frame " << positionInBuffer << " to video Buffer." << std::endl;
    //std::cout << "_NewFrame dims: " << _newFrame->rows << ", " << _newFrame->cols << std::endl;
    cv::Mat newFrame = _newFrame->t();
    newFrame = newFrame.reshape(0, 1);
    //std::cout << "NewFrame dims: " << newFrame.rows << ", " << newFrame.cols << std::endl;
    //std::cout << "videoBuffer dims: " << videoBuffer.rows << ", " << videoBuffer.cols << std::endl;
    //std::cout << "videoBuffer dims: " << videoBuffer.row(positionInBuffer).rows << ", " << videoBuffer.row(positionInBuffer).cols << std::endl;
    newFrame.copyTo(videoBuffer.row(positionInBuffer));
    // increase position in buffer
    ++positionInBuffer;
    if (positionInBuffer >= framesInBuffer) {
        positionInBuffer = positionInBuffer % framesInBuffer;
        initializing = false;
    }
    //std::cout << "Done editing, next frame would be " << positionInBuffer << std::endl;
}

void DotDetectorM::detectDots()
{
    // create projection matrix, that is the matrix with cos and sin values
    //arma::mat cosProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    //arma::mat sinProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    //for (unsigned int i(0); i < WDD_FBUFFER_SIZE; ++i) {
    //	for (unsigned int j(0); j < WDD_FREQ_NUMBER; ++j) {
    //		cosProjection.at(i, j) = DotDetectorLayer::SAMPLES[i].cosines[j];
    //		sinProjection.at(i, j) = DotDetectorLayer::SAMPLES[i].sines[j];
    //	}
    //}
    // perform projection
    //std::cout << "dimsVB rows: " << videoBuffer.n_rows << " cols: " << videoBuffer.n_cols << std::endl;
    //std::cout << "dimsCOS rows: " << cosProjection.n_rows << " cols: " << cosProjection.n_cols << std::endl;
    //arma::mat resultCos = videoBuffer * cosProjection;
    //arma::mat resultSin = videoBuffer * sinProjection;

    cv::Mat_<float> cosProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    cv::Mat_<float> sinProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    for (int i(0); i < WDD_FBUFFER_SIZE; ++i) {
        for (int j(0); j < WDD_FREQ_NUMBER; ++j) {
            cosProjection(i, j) = DotDetectorLayer::SAMPLES[i].cosines[j];
            sinProjection(i, j) = DotDetectorLayer::SAMPLES[i].sines[j];
        }
    }

    //std::cout << "videoBufferSize: " << videoBuffer.rows << ", " << videoBuffer.cols << std::endl;
    //std::cout << "CosProjectionSize: " << cosProjection.rows << ", " << cosProjection.cols << std::endl;
    //// perform projection for the whole buffer

    cv::Mat resultCos = videoBuffer.t() * cosProjection;
    cv::Mat resultSin = videoBuffer.t() * sinProjection;
    //std::cout << "resultCos: " << resultCos.rows << ", " << resultCos.cols << std::endl;

    for (int i(0); i < videoBuffer.cols; ++i) {
        const auto sin = resultSin.row(i);
        const auto cos = resultCos.row(i);
        executeDetection(sin, cos, i);
    }
    //cv::Mat_<float> potentials(resolutionX, resolutionY);
    //for (unsigned int i(0); i < resolutionX; ++i) {
    //	for (unsigned int j(0); j < resolutionY; ++j) {
    //		ranges[0] = cv::Range(i, i + 1);
    //		ranges[1] = cv::Range(j, j + 1);
    //		//cv::Mat_<float> timeSlice = videoBuffer(ranges);
    //		cv::Mat_<float> timeSlice;
    //		// this is the projection on sine and cosine
    //		cv::transpose(timeSlice, timeSlice);
    //		auto projectionsSin = timeSlice * sinProjection;
    //		auto projectionsCos = timeSlice * cosProjection;
    //		uint16_t id = j + i*resolutionY;
    //		executeDetection(projectionsSin, projectionsCos, id);
    //	}
    //}
}

inline void DotDetectorM::executeDetection(cv::Mat const& _projectedSin, cv::Mat const& _projectedCos, uint16_t _id)
{
    float fac = 200;
    // score_i = sinSum_i^2 + cosSum_i^2
    std::vector<float> scores;
    for (int i(0); i < WDD_FREQ_NUMBER; ++i) {
        float tmpSin = _projectedSin.at<float>(0, i) * _projectedSin.at<float>(0, i);
        float tmpCos = _projectedCos.at<float>(0, i) * _projectedCos.at<float>(0, i);
        //std::cout << "Sin: " << tmpSin << " Cos: " << tmpCos << std::endl;
        //std::cout << squaresSin[i] << " i: " << i << std::endl;
        //squaresSin[i] = 3;
        scores.push_back(tmpSin + tmpCos);
        //std::cout << "done" << std::endl;
    }
    //std::cout << "blub" << std::endl;
    //auto scoresForFrequencies = squaresSin + squaresCos;
    //std::cout << "Scores calculated!" << std::endl;
    double buff;
    buff = scores[0] + scores[1] + scores[2];
    //std::cout << "buff: " << buff << std::endl;
    for (int i = 3; i < WDD_FREQ_NUMBER; i++) {
        if (buff > fac * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
            DotDetectorLayer::DD_SIGNALS_NUMBER++;
            DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
            return;
        }
        buff += -scores[i - 3] + scores[i];
    }
    //std::cout << "buff: " << buff << " comp: " << 0.3 * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE << std::endl;
    if (buff > fac * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
        DotDetectorLayer::DD_SIGNALS_NUMBER++;
        DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
        return;
    }
}
