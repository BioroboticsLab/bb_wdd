#pragma once

#include "Camera.h"
#include "Config.h"
#include <string>
#include <thread>

namespace wdd {
class CLEyeCameraCapture {
    std::unique_ptr<Camera> _camera;
    std::string _windowName;
    std::string _cameraGUID;
    float _fps;
    bool _running;

    int _FRAME_WIDTH;
    int _FRAME_HEIGHT;
    bool _visual;
    bool _setupModeOn;

    size_t _cameraIdx;

    CamConf _CC;

    double aux_DD_MIN_POTENTIAL;
    int aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;

    std::thread _thread;

    void findCornerPointNear(cv::Point2i p) const;
    void onMouseInput(int evnt, int x, int y, int flags);
    static void onMouseInput(int evnt, int x, int y, int flags, void* userData);

public:
    CLEyeCameraCapture(std::string windowName, std::string cameraGUID, size_t cameraIdx, size_t width, size_t height, float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size);

    bool StartCapture();
    void StopCapture();

    void setVisual(bool visual);
    void setSetupModeOn(bool setupMode);
    const CamConf* getCamConfPtr();

    void handleParameterQueue();
    void Setup();

    void Run();

    void drawArena(cv::Mat& frame);
    void drawPosDDs(cv::Mat& frame);
    void makeHeartBeatFile();
    bool pointIsInArena(cv::Point p);
    double computeProduct(cv::Point p, cv::Point2i a, cv::Point2i b);
    size_t CaptureThread();
};
}
