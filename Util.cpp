#include "Util.h"

#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

double uvecToDegree(cv::Point2d in)
{
    if (std::isnan(in.x) | std::isnan(in.y))
        return std::numeric_limits<double>::quiet_NaN();

    // flip y-axis to match unit-circle-axis with image-matrice-axis
    double theta = atan2(-in.y, in.x);
    return theta * 180 / CV_PI;
}

std::string exec(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
        throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}
