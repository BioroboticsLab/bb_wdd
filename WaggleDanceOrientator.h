#pragma once
#include "cvblob.h"

namespace wdd {
class WaggleDanceOrientator {
public:
    static cv::Point2d extractOrientationFromPositions(std::vector<cv::Point2d> positions, cv::Point2d position_last);
    static cv::Point2d extractNaive(std::vector<cv::Point2d> positions, cv::Point2d position_last);

    static void saveImage(const cv::Mat* img_ptr, const char* path_ptr);
};
} /* namespace WaggleDanceDetector */
