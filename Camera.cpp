#include "Camera.h"

#include <stdexcept>

#include "Util.h"

Camera::Camera(int cameraIndex, double frameRate, double frameWidth, double frameHeight)
    : _cameraIndex(cameraIndex)
    , _cap(cameraIndex)
{
    if (!_cap.isOpened()) {
        throw std::runtime_error("Camera device not opened successfully;");
    }

    setCaptureProps(frameRate, frameWidth, frameHeight);

    // make sure frame pointer is initialized correctly
    nextFrame();
}

void Camera::nextFrame()
{
    _cap >> _frame;
    cv::cvtColor(_frame, _frame, CV_BGR2GRAY);
}

cv::Mat* Camera::getFramePointer()
{
    return &_frame;
}

void Camera::setCaptureProps(double frameRate, double frameWidth, double frameHeight)
{
    _cap.set(CV_CAP_PROP_FPS, frameRate);
    _cap.set(CV_CAP_PROP_FRAME_WIDTH, frameWidth);
    _cap.set(CV_CAP_PROP_FRAME_HEIGHT, frameHeight);
    _cap.set(CV_CAP_PROP_AUTO_EXPOSURE, false);
}

std::string Camera::getIdentifier(int cameraIndex)
{
    // As far as I can tell, there's no way to get a unique identifier for PS3Eye cameras on linux.
    // This method return a identifier that is unique for each USB port, therefore the same camera should also be
    // connected to the same USB port.
    std::stringstream ss;
    ss << "udevadm info -a -p $(udevadm info -q path -p /class/video4linux/video";
    ss << std::to_string(cameraIndex);
    ss << ") | grep KERNELS | head -n 1 | grep -oP '\"\\K[^\"\\047]+(?=[\"\\047])' | xargs";
    std::string cmd = ss.str();
    std::string ret = exec(cmd.data());
    trim(ret);
    return ret;
}

size_t Camera::getNumCameras()
{
    static const int MAX_NUM_CAMERAS = 4;

    // temporary redirect stderr to /dev/null to silence VFL2 errors while enumerating
    // potentially unconnected camera devices
    freopen("/dev/null", "w", stderr);
    size_t cnt = 0;
    for (int idx = 0; idx <= MAX_NUM_CAMERAS; ++idx) {
        auto cap = cv::VideoCapture(idx);
        if (cap.isOpened()) {
            cnt += 1;
        }
    }
    freopen("/dev/stdout", "w", stderr);

    return cnt;
}
