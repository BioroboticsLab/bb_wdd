#pragma once

#include "opencv2/opencv.hpp"

class Camera {
public:
    Camera(int cameraIndex, double frameRate, double frameWidth, double frameHeight);

    void nextFrame();
    cv::Mat* getFramePointer();

    static size_t getNumCameras();

private:
    cv::VideoCapture _cap;
    cv::Mat _frame;
};
