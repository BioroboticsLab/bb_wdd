#include "WaggleDanceOrientator.h"
#include "Config.h"
#include "cvblob.h"
#include "opencv2/opencv.hpp"

namespace wdd {

cv::Point2d WaggleDanceOrientator::extractOrientationFromPositions(std::vector<cv::Point2d> positions, cv::Point2d position_last)
{
    printf("extracting orientation\n");
    printf("size of positions: %d\n", (int)positions.size());
    cv::Vec4f direction;
    std::vector<cv::Point2f> positionsF;
    cv::Mat(positions).copyTo(positionsF);

    cv::fitLine(positionsF, direction, CV_DIST_L2, 0, 0.01, 0.01);
    cv::Point2d orient_raw = cv::Point2d(direction[0], direction[1]);
    printf("orientation by fitline: %f\n", atan2(orient_raw.y, orient_raw.x) * 180. / 3.14259);

    cv::Point2d orient_raw_naive = position_last - positions[0];

    printf("orientation by naive: %f\n", atan2(orient_raw_naive.y, orient_raw_naive.x) * 180. / 3.14259);
    if (cv::norm(orient_raw) > 0)
        return (orient_raw * (1.0 / cv::norm(orient_raw)));
    else
        return cv::Point2d(1, 1);
}

cv::Point2d WaggleDanceOrientator::extractNaive(std::vector<cv::Point2d> positions, cv::Point2d position_last)
{

    cv::Point2d orient_raw = position_last - positions[0];
    if (cv::norm(orient_raw) > 0)
        return (orient_raw * (1.0 / cv::norm(orient_raw)));
    else
        return cv::Point2d(1, 1);
}

void WaggleDanceOrientator::saveImage(const cv::Mat* img_ptr, const char* path_ptr)
{
    bool result;

    try {

        result = cv::imwrite(path_ptr, *img_ptr);
    } catch (std::runtime_error& ex) {
        std::cerr << "Exception saving image '" << path_ptr
                  << "': " << ex.what() << std::endl;
        result = false;
    }
    if (!result)
        std::cerr << "Error saving image '" << path_ptr << "'!" << std::endl;
}
} /* namespace WaggleDanceDetector */
