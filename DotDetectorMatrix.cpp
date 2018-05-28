#include "DotDetectorMatrix.h"
#include "Config.h"
#include "DotDetectorLayer.h"

using namespace wdd;

cv::Mat DotDetectorM::videoBuffer;
unsigned int DotDetectorM::positionInBuffer;
bool DotDetectorM::initializing;
unsigned int DotDetectorM::resolutionX, DotDetectorM::resolutionY, DotDetectorM::framesInBuffer;

void DotDetectorM::init(unsigned int _resX, unsigned int _resY, unsigned int _framesInBuffer)
{
    std::cout << "Initializing dot detector" << std::endl;
    videoBuffer = cv::Mat(_framesInBuffer, _resX * _resY, CV_32F);
    positionInBuffer = 0;
    initializing = true;
    resolutionX = _resX;
    resolutionY = _resY;
    framesInBuffer = _framesInBuffer;
    std::cout << "Initialization complete" << std::endl;
}

void DotDetectorM::addNewFrame(cv::Mat* _newFrame)
{
    cv::Mat newFrame = _newFrame->t();
    newFrame = newFrame.reshape(0, 1);
    newFrame.copyTo(videoBuffer.row(positionInBuffer));

    // increase position in buffer
    ++positionInBuffer;
    if (positionInBuffer >= framesInBuffer) {
        positionInBuffer = positionInBuffer % framesInBuffer;
        initializing = false;
    }
}

void DotDetectorM::detectDots()
{
    cv::Mat_<float> cosProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    cv::Mat_<float> sinProjection(WDD_FBUFFER_SIZE, WDD_FREQ_NUMBER);
    for (int i(0); i < WDD_FBUFFER_SIZE; ++i) {
        for (int j(0); j < WDD_FREQ_NUMBER; ++j) {
            cosProjection(i, j) = DotDetectorLayer::SAMPLES[i].cosines[j];
            sinProjection(i, j) = DotDetectorLayer::SAMPLES[i].sines[j];
        }
    }

    // perform projection for the whole buffer
    cv::Mat resultCos = videoBuffer.t() * cosProjection;
    cv::Mat resultSin = videoBuffer.t() * sinProjection;

    for (int i(0); i < videoBuffer.cols; ++i) {
        const auto sin = resultSin.row(i);
        const auto cos = resultCos.row(i);
        executeDetection(sin, cos, i);
    }
}

inline void DotDetectorM::executeDetection(cv::Mat const& _projectedSin, cv::Mat const& _projectedCos, uint16_t _id)
{
    std::vector<float> scores;
    for (int i(0); i < WDD_FREQ_NUMBER; ++i) {
        float tmpSin = _projectedSin.at<float>(0, i) * _projectedSin.at<float>(0, i);
        float tmpCos = _projectedCos.at<float>(0, i) * _projectedCos.at<float>(0, i);
        scores.push_back(tmpSin + tmpCos);
    }
    double buff = scores[0] + scores[1] + scores[2];
    for (int i = 3; i < WDD_FREQ_NUMBER; i++) {
        if (buff > DD_DETECTION_FACTOR * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
            DotDetectorLayer::DD_SIGNALS_NUMBER++;
            DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
            return;
        }
        buff += -scores[i - 3] + scores[i];
    }
    if (buff > DD_DETECTION_FACTOR * WDD_FBUFFER_SIZE * WDD_FBUFFER_SIZE) {
        DotDetectorLayer::DD_SIGNALS_NUMBER++;
        DotDetectorLayer::DD_SIGNALS_IDs.push_back(_id);
        return;
    }
}
