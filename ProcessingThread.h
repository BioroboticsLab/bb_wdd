#pragma once

#include "Camera.h"
#include "Config.h"
#include "WaggleDanceDetector.h"
#include <string>
#include <thread>

namespace wdd {
class ProcessingThread {
private:
    size_t _cameraIdx;
    std::unique_ptr<Camera> _camera;
    MouseInteraction _mouse;
    std::unique_ptr<WaggleDanceDetector> _wdd;

    std::string _windowName;
    std::string _cameraGUID;
    float _fps;
    bool _running;

    bool _visual;
    int _frameWidth;
    int _frameHeight;
    bool _setupModeOn;

    int aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;

    CamConf _camConfig;

    double aux_DD_MIN_POTENTIAL;

    const std::string _dancePath;

    void findCornerPointNear(cv::Point2i p);
    void onMouseInput(int evnt, int x, int y);
    static void onMouseInput(int evnt, int x, int y, int flags, void* userData);

public:
    ProcessingThread(std::string windowName, std::string cameraGUID, size_t cameraIdx, size_t width, size_t height,
        float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size,
        std::string const& dancePath);

    bool StartCapture();
    void StopCapture();

    void setVisual(bool VISUAL);
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
