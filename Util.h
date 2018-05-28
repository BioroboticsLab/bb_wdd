#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>

using namespace std; //Don't if you're in a header-file

double uvecToDegree(cv::Point2d in);

std::string exec(const char* cmd);

void ltrim(std::string& s);
void rtrim(std::string& s);
void trim(std::string& s);

std::string getNameOfExe();
std::string getExeFullPath();
bool fileExists(const std::string& file_name);
bool dirExists(const char* dirPath);

template <typename... Args>
std::string string_format(const std::string& format, Args... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
