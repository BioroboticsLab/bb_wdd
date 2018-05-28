#include "WaggleDanceExport.h"
#include "Config.h"
#include "Util.h"
#include "WaggleDanceOrientator.h"
#include "opencv2/opencv.hpp"

#include <boost/filesystem.hpp>

namespace wdd {
static const std::string rootPath = "output";

// save the current day
static std::string buf_YYYYMMDD;
// save the current hour:minute and CamID 0-9
static std::string buf_YYYYMMDD_HHMM_camID;
static std::string relpath_YYYYMMDD_HHMM_camID;
static std::string buf_camID;
static std::string buf_dirID;

static std::array<cv::Point2i, 4> auxArena;

void WaggleDanceExport::write(const std::vector<cv::Mat> seq, const Dance* d_ptr, std::size_t camID)
{
    struct tm* timeinfo;
    timeinfo = localtime(&d_ptr->rawTime.tv_sec);
    const long milliSeconds = d_ptr->rawTime.tv_usec / 1000;

    // check <YYYYYMMDD> folder
    // write YYYYYMMDD string & compare to last saved
    std::string date = string_format("%04d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon, timeinfo->tm_mday);

    if (buf_YYYYMMDD != date) {
        // if different, save it and ensure directory exists
        buf_YYYYMMDD = date;
        WaggleDanceExport::createGenericFolder(buf_YYYYMMDD);
    }

    // check <YYYYYMMDD_HHMM_CamID> folder
    // write YYYYYMMDD_HHMM_ string
    std::string timestamp = string_format("%04d%02d%02d_%02d%02d_",
        timeinfo->tm_year + 1900, timeinfo->tm_mon, timeinfo->tm_mday,
        timeinfo->tm_hour, timeinfo->tm_min);

    // convert camID to string
    buf_camID = std::to_string(camID);
    // append and finalize YYYYYMMDD_HHMM_camID string
    std::string timestampWithCamId = timestamp + buf_camID;

    if (timestampWithCamId != buf_YYYYMMDD_HHMM_camID) {
        // if different, save it and ensure directory exists
        buf_YYYYMMDD_HHMM_camID = timestampWithCamId;

        boost::filesystem::path relPath(buf_YYYYMMDD);
        relPath /= buf_YYYYMMDD_HHMM_camID;
        WaggleDanceExport::createGenericFolder(relPath.string());

        relpath_YYYYMMDD_HHMM_camID = relPath.string();
    }

    // check <ID> folder
    // get dir <ID>
    std::size_t dirID = countDirectories(relpath_YYYYMMDD_HHMM_camID);

    // convert dirID to string
    buf_dirID = std::to_string(dirID);

    boost::filesystem::path relPath(relpath_YYYYMMDD_HHMM_camID);
    relPath /= buf_dirID;

    WaggleDanceExport::createGenericFolder(relPath.string());

    // write CSV file
    // link full path from main.cpp
    boost::filesystem::path csvFile(getExeFullPath());
    csvFile /= rootPath;
    csvFile /= relPath;

    std::string csvPath(csvFile.string());
    csvPath += buf_YYYYMMDD_HHMM_camID;
    csvPath += "_";
    csvPath += buf_dirID;
    csvPath += ".csv";

    std::cout << csvPath << std::endl;

    FILE* CSV_ptr = fopen(csvPath.c_str(), "w");

    char TIMESTMP[32];
    sprintf(TIMESTMP, "%02d:%02d:%02d:%03d",
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, milliSeconds);

    fprintf(CSV_ptr, "%.1f %.1f %.1f\n", d_ptr->positions[0].x, d_ptr->positions[0].y, uvecToRad(d_ptr->orientUVec));
    fprintf(CSV_ptr, "%s %d\n", TIMESTMP, static_cast<int>(d_ptr->danceFrameEnd - d_ptr->danceFrameStart + 1));

    for (unsigned i = 0; i < 4; i++) {
        fprintf(CSV_ptr, "%d %d ", auxArena[i].x, auxArena[i].y);
    }
    fprintf(CSV_ptr, "\n");

    for (auto it = d_ptr->positions.begin(); it != d_ptr->positions.end(); ++it) {
        fprintf(CSV_ptr, "%.1f %.1f ", it->x, it->y);
    }
    fprintf(CSV_ptr, "\n");
    fclose(CSV_ptr);

    // write images
    // get image dimensions
    cv::Size size = seq[0].size();

    // get direction length
    double length = MIN(size.width / 2, size.height / 2) * 0.8;

    // get image center point
    cv::Point2d CENTER(size.width / 2., (size.height / 2.));

    // get image orientation point
    cv::Point2d HEADIN(CENTER);
    HEADIN += d_ptr->orientUVec * length;

    // preallocate 3 channel image output buffer
    cv::Mat image_out(size, CV_8UC3);

    std::size_t i = 0;
    for (auto it = seq.begin(); it != seq.end(); ++it) {
        // convert & copy input image into BGR
        cv::cvtColor(*it, image_out, CV_GRAY2BGR);

        // create dynamic path_out string
        boost::filesystem::path path(getExeFullPath());
        path /= rootPath;
        path /= relpath_YYYYMMDD_HHMM_camID;
        path /= buf_dirID;

        std::string uid = string_format("%03d", i);

        std::string filename = "image_" + uid + ".png";
        path /= filename;

        // write image to disk
        WaggleDanceOrientator::saveImage(&image_out, path.c_str());
        i++;
    }

    //finally draw detected orientation
    //black background
    image_out.setTo(0);

    // draw the orientation line
    cv::line(image_out, CENTER, HEADIN, CV_RGB(0., 255., 0.), 2, CV_AA);

    // create dynamic path_out string
    boost::filesystem::path path(getExeFullPath());
    path /= rootPath;
    path /= relpath_YYYYMMDD_HHMM_camID;
    path /= buf_dirID;
    path /= "orient.png";

    WaggleDanceOrientator::saveImage(&image_out, path.c_str());
}

double WaggleDanceExport::uvecToRad(cv::Point2d in)
{
    if (std::isnan(in.x) | std::isnan(in.y))
        return std::numeric_limits<double>::quiet_NaN();

    return atan2(in.y, in.x);
}
void WaggleDanceExport::setArena(std::array<cv::Point2i, 4> _auxArena)
{
    auxArena = _auxArena;
}
void WaggleDanceExport::execRootExistChk()
{

    WaggleDanceExport::createGenericFolder("");
}

void WaggleDanceExport::createGenericFolder(std::string const& dir_rel)
{
    boost::filesystem::path path(getExeFullPath());
    path /= rootPath;
    path /= dir_rel;

    if (!boost::filesystem::is_directory(path)) {
        if (!boost::filesystem::create_directory(path)) {
            printf("ERROR! Couldn't create %s directory.\n", path.c_str());
            exit(-19);
        }
    }
}

long WaggleDanceExport::countDirectories(std::string const& dir_rel)
{
    boost::filesystem::path path(getExeFullPath());
    path /= rootPath;
    path /= dir_rel;

    const long cnt = std::count_if(
        boost::filesystem::directory_iterator(path),
        boost::filesystem::directory_iterator(),
        static_cast<bool (*)(const boost::filesystem::path&)>(boost::filesystem::is_directory));

    return cnt;
}
}
