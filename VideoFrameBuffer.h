#pragma once

#include <opencv2/opencv.hpp>

#include "Config.h"
#include "Util.h"

namespace wdd {
class VideoFrameBuffer {
    unsigned int _bufferPos;
    unsigned long long _currentFrameNumber;

    cv::Size _cachedFrameSize;
    cv::Size _extractFrameSize;
    cv::Point _sequenceFramePointOffset;

    cv::Mat _frames[VFB_MAX_FRAME_HISTORY];
    time_t _rawTime;
    CamConf _camConf;
    int _lastHour;

public:
    VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size cachedFrameSize, cv::Size extractFrameSize, CamConf _camConf);

    void setSequecenFrameSize(cv::Size size);
    void addFrame(cv::Mat* frame_ptr);
    void saveFullFrame();
    void drawArena(cv::Mat& frame);
    cv::Mat* getFrameByNumber(unsigned long long frame_nr);
    std::vector<cv::Mat> loadCroppedFrameSequence(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC);
};
} /* namespace WaggleDanceDetector */
