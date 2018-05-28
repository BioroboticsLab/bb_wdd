#include "Util.h"

#include <algorithm>
#include <array>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <locale>
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

void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void rtrim(std::string& s)
{
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        })
            .base(),
        s.end());
}

void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

std::string getNameOfExe()
{
    return boost::dll::program_location().stem().string();
}

std::string getExeFullPath()
{
    return boost::dll::program_location().parent_path().string();
}

bool fileExists(const std::string& file_name)
{
    return boost::filesystem::is_regular_file(boost::filesystem::path(file_name));
}

bool dirExists(const char* dirPath)
{
    return boost::filesystem::is_directory(boost::filesystem::path(dirPath));
}
