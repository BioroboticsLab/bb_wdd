#pragma once

#include "opencv2/opencv.hpp"

class Camera {
public:
    Camera(int cameraIndex, double frameRate, double frameWidth, double frameHeight);

    void nextFrame();
    cv::Mat* getFramePointer();
    bool isOpened() const { return _cap.isOpened(); }
    void setCaptureProps(double frameRate, double frameWidth, double frameHeight);

    static std::string getIdentifier(int cameraIndex);
    static size_t getNumCameras();

private:
    const int _cameraIndex;
    cv::VideoCapture _cap;
    cv::Mat _frame;
};
