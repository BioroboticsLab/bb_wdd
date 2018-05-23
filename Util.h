#pragma once

#include <opencv2/opencv.hpp>

double uvecToDegree(cv::Point2d in);

std::string exec(const char* cmd);

void ltrim(std::string& s);
void rtrim(std::string& s);
void trim(std::string& s);
