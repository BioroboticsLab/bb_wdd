#include "Util.h"

double uvecToDegree(cv::Point2d in)
{
    if(std::isnan(in.x) | std::isnan(in.y))
        return std::numeric_limits<double>::quiet_NaN();

    // flip y-axis to match unit-circle-axis with image-matrice-axis
    double theta = atan2(-in.y,in.x);
    return theta * 180/CV_PI;
}
