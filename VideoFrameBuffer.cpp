#include "VideoFrameBuffer.h"
#include "Config.h"
#include "Util.h"
#include "WaggleDanceOrientator.h"

#include <boost/filesystem.hpp>

namespace wdd {

VideoFrameBuffer::VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size cachedFrameSize, cv::Size extractFrameSize, CamConf _CC)
    : _bufferPos(0)
    , _currentFrameNumber(current_frame_nr)
    , _cachedFrameSize(cachedFrameSize)
    , _extractFrameSize(extractFrameSize)
    , _camConf(_CC)
    , _lastHour(-1)
{
    if ((extractFrameSize.width > cachedFrameSize.width) || (extractFrameSize.height > cachedFrameSize.height)) {
        std::cerr << "Error! VideoFrameBuffer():: extractFrameSize can not be bigger then cachedFrameSize in any dimension!" << std::endl;
    }
    _sequenceFramePointOffset = cv::Point2i(extractFrameSize.width / 2, extractFrameSize.height / 2);

    //init memory
    for (std::size_t i = 0; i < VFB_MAX_FRAME_HISTORY; i++) {
        _frames[i] = cv::Mat(cachedFrameSize, CV_8UC1);
    }
}

void VideoFrameBuffer::addFrame(cv::Mat* frame_ptr)
{
    _frames[_bufferPos] = frame_ptr->clone();

    if (_currentFrameNumber > 1000) {
        struct tm* timeinfo;
        time(&_rawTime);
        timeinfo = localtime(&_rawTime);

        int curent_hour = timeinfo->tm_hour;
        if ((curent_hour > _lastHour) || ((curent_hour == 0) && (_lastHour == 23)))

        {
            _lastHour = curent_hour;
            saveFullFrame();
        }
    }
    _bufferPos = (++_bufferPos) < VFB_MAX_FRAME_HISTORY ? _bufferPos : 0;

    _currentFrameNumber++;

    // check for hourly save image
}
void VideoFrameBuffer::saveFullFrame()
{
    boost::filesystem::path path(getExeFullPath());
    path /= VFB_ROOT_PATH;

    // check for path_out folder
    if (!dirExists(path.c_str())) {
        if (!boost::filesystem::create_directory(path)) {
            printf("ERROR! Couldn't create %s directory.\n", path.c_str());
            exit(-19);
        }
    }

    path /= std::to_string(_camConf.camId);

    // check for path_out//id folder
    if (!dirExists(path.c_str())) {
        if (!boost::filesystem::create_directory(path)) {
            printf("ERROR! Couldn't create %s directory.\n", path.c_str());
            exit(-19);
        }
    }

    // create dynamic path_out string
    struct tm* timeinfo;
    timeinfo = localtime(&_rawTime);
    char TIMESTMP[64];
    sprintf(TIMESTMP, "%04d%02d%02d_%02d%02d_",
        timeinfo->tm_year + 1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
    std::string timestamp(TIMESTMP);

    path /= timestamp;

    std::string finalPath(path.string());
    finalPath += std::to_string(_camConf.camId);
    finalPath += ".png";

    cv::Mat _tmp = _frames[_bufferPos].clone();

    cv::cvtColor(_tmp, _tmp, CV_GRAY2BGR);
    drawArena(_tmp);

    WaggleDanceOrientator::saveImage(&_tmp, finalPath.c_str());
}
void VideoFrameBuffer::drawArena(cv::Mat& frame)
{
    float fac_red = 1.0 / 2.0;
    for (std::size_t i = 0; i < _camConf.arena.size(); i++) {
        cv::line(frame,
            cv::Point2f(_camConf.arena[i].x * fac_red, _camConf.arena[i].y * fac_red),
            cv::Point2f(_camConf.arena[(i + 1) % _camConf.arena.size()].x * fac_red, _camConf.arena[(i + 1) % _camConf.arena.size()].y * fac_red), CV_RGB(0, 255, 0));
    }
}
cv::Mat* VideoFrameBuffer::getFrameByNumber(unsigned long long req_frame_nr)
{
    int frameJump = static_cast<int>(_currentFrameNumber - req_frame_nr);

    // if req_frame_nr > CURRENT_FRAME_NR --> framesJump < 0
    if (frameJump <= 0) {
        std::cerr << "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame from future " << req_frame_nr << " (CURRENT_FRAME_NR < req_frame_nr)!" << std::endl;
        return NULL;
    }

    // assert: CURRENT_FRAME_NR >= req_frame_nr --> framesJump >= 0
    if (frameJump <= VFB_MAX_FRAME_HISTORY) {
        int framePosition = _bufferPos - frameJump;

        framePosition = framePosition >= 0 ? framePosition : VFB_MAX_FRAME_HISTORY + framePosition;

        //std::cout<<"VideoFrameBuffer::getFrameByNumber::_BUFFER_POS: "<<framePosition<<std::endl;

        return &_frames[framePosition];
    } else {
        std::cerr << "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame " << req_frame_nr << " - history too small!" << std::endl;
        return NULL;
    }
}

std::vector<cv::Mat> VideoFrameBuffer::loadCroppedFrameSequence(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC)
{
    // fatal: if this is true, return empty vector
    if (startFrame >= endFrame)
        return std::vector<cv::Mat>();

    const unsigned short _center_x = static_cast<int>(center.x * pow(2, FRAME_REDFAC));
    const unsigned short _center_y = static_cast<int>(center.y * pow(2, FRAME_REDFAC));

    int roi_rec_x = static_cast<int>(_center_x - _sequenceFramePointOffset.x);
    int roi_rec_y = static_cast<int>(_center_y - _sequenceFramePointOffset.y);

    // safe roi edge - lower bounds check
    roi_rec_x = roi_rec_x < 0 ? 0 : roi_rec_x;
    roi_rec_y = roi_rec_y < 0 ? 0 : roi_rec_y;

    // safe roi edge - upper bounds check
    roi_rec_x = (roi_rec_x + _extractFrameSize.width) <= _cachedFrameSize.width ? roi_rec_x : _cachedFrameSize.width - _extractFrameSize.width;
    roi_rec_y = (roi_rec_y + _extractFrameSize.height) <= _cachedFrameSize.height ? roi_rec_y : _cachedFrameSize.height - _extractFrameSize.height;

    // set roi
    cv::Rect roi_rec(cv::Point2i(roi_rec_x, roi_rec_y), _extractFrameSize);

    std::vector<cv::Mat> out;

    cv::Mat* frame_ptr;

    while (startFrame <= endFrame) {
        frame_ptr = getFrameByNumber(startFrame);

        // check the pointer is not null and cv::Mat not empty
        if (frame_ptr && !frame_ptr->empty()) {
            cv::Mat subframe_monochrome(*frame_ptr, roi_rec);
            out.push_back(subframe_monochrome.clone());
        } else {
            std::cerr << "Error! VideoFrameBuffer::loadFrameSequenc frame " << startFrame << " empty!" << std::endl;
            return std::vector<cv::Mat>();
        }
        startFrame++;
    }

    return out;
}
}
