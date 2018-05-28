#include "ProcessingThread.h"
#include "Config.h"
#include "DotDetectorLayer.h"
#include "Util.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"

#include <boost/filesystem.hpp>
#include <cstdio>

namespace wdd {

void ProcessingThread::findCornerPointNear(cv::Point2i p)
{
    // we actually have a 4 point arena to work with
    if (_camConfig.arena.size() != 4)
        printf("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _camConfig.arena.size());

    // check if there is already a corner marked as hovered
    if (_mouse.cornerHovered > -1) {
        // revalidate distance
        if (cv::norm(_camConfig.arena[_mouse.cornerHovered] - cv::Point2i(p)) < 25) {
            // nothing to do, no GUI update needed
            return;
        } else {
            // unset corner marked as hovered
            _mouse.cornerHovered = -1;
            // done
            return;
        }
    }

    //find the corner the mouse points at
    // assertion Mouse.cornerHover = -1;
    for (std::size_t i = 0; i < _camConfig.arena.size(); i++) {
        if (cv::norm(_camConfig.arena[i] - cv::Point2i(p)) < 25) {
            _mouse.cornerHovered = i;
            break;
        }
    }
}
void ProcessingThread::onMouseInput(int evnt, int x, int y)
{
    cv::Point2i p(x, y);
    _mouse.lastPosition = p;

    switch (evnt) {
    case CV_EVENT_MOUSEMOVE:
        // if a corner is selected, move the edge position
        if (_mouse.cornerSelected > -1) {
            _camConfig.arena[_mouse.cornerSelected] = p;
        } else {
            // hover over corner point should trigger visual feedback
            findCornerPointNear(p);
        }
        break;

    case CV_EVENT_LBUTTONDOWN:
        // implement toggle: if corner was already selected, deselect it
        if (_mouse.cornerSelected > -1)
            _mouse.cornerSelected = -1;
        // else check if corner is hovered (=in range) and possibly select it
        else if (_mouse.cornerHovered > -1)
            _mouse.cornerSelected = _mouse.cornerHovered;
        break;
    }
}
void ProcessingThread::onMouseInput(int evnt, int x, int y, int, void* userData)
{
    ProcessingThread* self = reinterpret_cast<ProcessingThread*>(userData);
    self->onMouseInput(evnt, x, y);
}
ProcessingThread::ProcessingThread(std::string windowName, std::string cameraGUID, size_t cameraIdx, size_t width, size_t height, float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size, const std::string& dancePath)
    : _windowName(windowName)
    , _cameraGUID(cameraGUID)
    , _cameraIdx(cameraIdx)
    , _camera(std::make_unique<Camera>(cameraIdx, fps, width, height))
    , _fps(fps)
    , _running(false)
    , _visual(true)
    , _frameWidth(width)
    , _frameHeight(height)
    , aux_DD_MIN_POTENTIAL(dd_min_potential)
    , aux_WDD_SIGNAL_MIN_CLUSTER_SIZE(wdd_signal_min_cluster_size)
    , _camConfig(CC)
    , _dancePath(dancePath)
{
    _setupModeOn = !_camConfig.configured;
}
bool ProcessingThread::StartCapture()
{
    _running = true;

    cv::namedWindow(_windowName);

    CaptureThread();

    return true;
}
void ProcessingThread::setVisual(bool visual)
{
    _visual = visual;
}
void ProcessingThread::setSetupModeOn(bool setupMode)
{
    _setupModeOn = setupMode;
}
const CamConf* ProcessingThread::getCamConfPtr()
{
    return &_camConfig;
}
void ProcessingThread::Setup()
{
    // listen for mouse interaction
    cv::setMouseCallback(_windowName, onMouseInput, this);
    // id der Ecke, die mit der Maus angehovert wurde
    _mouse.cornerHovered = -1;
    // id der Ecke, die mit der Maus angeklickt wurde
    _mouse.cornerSelected = -1;

    // Create camera instance
    _camera->setCaptureProps(30, 640, 480);
    if (!_camera->isOpened()) {
        std::cerr << "ERROR! Could not create camera instance!" << std::endl;
        return;
    }
    /*
    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_GAIN = true!" << std::endl;

    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_EXPOSURE = true!" << std::endl;

    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_WHITEBALANCE = true!" << std::endl;
    */
    // TODO BEN: FIXME

    // image capturing loop
    cv::Mat* framePtr = _camera->getFramePointer();
    // run until user presses Esc
    while (cv::waitKey(25) != 27) {
        _camera->nextFrame();

        cv::Mat frame_input_monochrome(*framePtr);
        cv::cvtColor(frame_input_monochrome, frame_input_monochrome, CV_GRAY2BGR);
        drawArena(frame_input_monochrome);
        cv::imshow(_windowName, frame_input_monochrome);
        cv::waitKey(1);
    }

    // set CamConf to configured
    _camConfig.configured = true;
}
void ProcessingThread::drawArena(cv::Mat& frame)
{
    // we actually have a 4 point arena to work with
    if (_camConfig.arena.size() != 4) {
        printf("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _camConfig.arena.size());
    }
    // setupModeOn -> expect 640x480
    if (_setupModeOn) {
        for (std::size_t i = 0; i < _camConfig.arena.size(); i++) {
            cv::line(frame, _camConfig.arena[i], _camConfig.arena[(i + 1) % _camConfig.arena.size()], CV_RGB(0, 255, 0), 2, CV_AA);
            cv::circle(frame, _camConfig.arena[i], 5, CV_RGB(0, 255, 0), 2, CV_AA);

            if (_mouse.cornerSelected > -1)
                cv::circle(frame, _camConfig.arena[_mouse.cornerSelected], 5, CV_RGB(255, 255, 255), 2, CV_AA);
        }
    }
    // else from 640x480 -> 180x120
    else {
        for (std::size_t i = 0; i < _camConfig.arena.size(); i++) {
            cv::line(frame, _camConfig.arena[i], _camConfig.arena[(i + 1) % _camConfig.arena.size()], CV_RGB(0, 255, 0), 2, CV_AA);
        }
    }
}
void ProcessingThread::drawPosDDs(cv::Mat& frame)
{
    for (auto it = DotDetectorLayer::DD_SIGNALS_IDs.begin(); it != DotDetectorLayer::DD_SIGNALS_IDs.end(); ++it) {
        cv::circle(frame, DotDetectorLayer::positions.at(*it), 1, CV_RGB(0, 255, 0));
    }
}

void ProcessingThread::makeHeartBeatFile()
{
    boost::filesystem::path path(getExeFullPath());
    path /= getNameOfExe();
    std::string pathStr(path.string());
    pathStr += HBF_EXTENSION;

    FILE* pFile = fopen(pathStr.c_str(), "w");

    if (pFile != nullptr)
        fclose(pFile);
    else
        std::cerr << "ERROR! Could not create heartbeat file: " << path << std::endl;
}

// Adoption from stackoverflow
// http://stackoverflow.com/questions/13080515/rotatedrect-roi-in-opencv
// decide whether point p is in the ROI.
// The ROI is a RotatableBox whose 4 corners are stored in pt[]
bool ProcessingThread::pointIsInArena(cv::Point p)
{
    return true;
    double result[4];
    for (int i = 0; i < 4; i++) {
        result[i] = computeProduct(p, _camConfig.arena_lowRes[i], _camConfig.arena_lowRes[(i + 1) % 4]);
        // all have to be 1, exit on first negative encounter
        if (result[i] < 0)
            return false;
    }
    if (result[0] + result[1] + result[2] + result[3] == 4)
        return true;
    else
        return false;
}
// Adoption from stackoverflow
// http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
// Use the sign of the determinant of vectors (AB,AM), where M(X,Y) is the query point:
double ProcessingThread::computeProduct(const cv::Point p, const cv::Point2i a, const cv::Point2i b)
{
    double x = ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x));

    return (x >= 0) ? 1 : -1;
}
void ProcessingThread::Run()
{
    std::cout << "Run" << std::endl;

    _camera->setCaptureProps(_fps, _frameWidth, _frameHeight);

    const double DD_MIN_POTENTIAL = aux_DD_MIN_POTENTIAL;
    const int WDD_SIGNAL_MIN_CLUSTER_SIZE = aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;

    // prepare frame_counter
    unsigned long long frame_counter_global = 0;

    // prepare videoFrameBuffer
    VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(_frameWidth, _frameHeight), cv::Size(20, 20), _camConfig);

    // prepare buffer for possible output
    cv::Mat frame_visual;

    // prepare buffer to hold target frame
    double resize_factor = pow(2.0, FRAME_RED_FAC);

    int frame_target_width = cvRound(_frameWidth / resize_factor);
    int frame_target_height = cvRound(_frameHeight / resize_factor);
    cv::Mat frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);

    std::cout << "Printing WaggleDanceDetector frame parameter:" << std::endl;
    printf("frame_height: %d\n", frame_target_height);
    printf("frame_width: %d\n", frame_target_width);
    printf("frame_rate: %d\n", WDD_FRAME_RATE);
    printf("frame_red_fac: %d\n", FRAME_RED_FAC);

    // prepare DotDetectorLayer config vector
    std::vector<double> ddl_config;
    ddl_config.push_back(DD_FREQ_MIN);
    ddl_config.push_back(DD_FREQ_MAX);
    ddl_config.push_back(DD_FREQ_STEP);
    ddl_config.push_back(WDD_FRAME_RATE);
    ddl_config.push_back(FRAME_RED_FAC);
    ddl_config.push_back(DD_MIN_POTENTIAL);

    // select DotDetectors according to Arena

    // assert: Setup() before Run() -> arena configured, 640x480 -> 180x120
    std::cout << _camConfig.arena[0] << std::endl;
    for (int i = 0; i < 4; i++) {
        _camConfig.arena_lowRes[i].x = static_cast<int>(std::floor(_camConfig.arena[i].x * 0.25));
        _camConfig.arena_lowRes[i].y = static_cast<int>(std::floor(_camConfig.arena[i].y * 0.25));
    }

    std::vector<cv::Point2i> dd_positions;
    for (int i = 0; i < frame_target_width; i++) {
        for (int j = 0; j < frame_target_height; j++) {
            // x (width), y(height)
            cv::Point2i tmp(i, j);
            if (pointIsInArena(tmp))
                dd_positions.push_back(tmp);
        }
    }
    printf("Initialize with %d DotDetectors (DD_MIN_POTENTIAL=%.1f) (WDD_SIGNAL_MIN_CLUSTER_SIZE=%d).\n",
        dd_positions.size(), DD_MIN_POTENTIAL, WDD_SIGNAL_MIN_CLUSTER_SIZE);

    // prepare WaggleDanceDetector config vector
    std::vector<double> wdd_config;
    // Layer 2
    wdd_config.push_back(WDD_SIGNAL_DD_MAXDISTANCE);
    wdd_config.push_back(WDD_SIGNAL_MIN_CLUSTER_SIZE);
    // Layer 3
    wdd_config.push_back(WDD_DANCE_MAX_POSITION_DISTANCEE);
    wdd_config.push_back(WDD_DANCE_MAX_FRAME_GAP);
    wdd_config.push_back(WDD_DANCE_MIN_CONSFRAMES);

    _wdd = std::make_unique<WaggleDanceDetector>(
        dd_positions,
        &frame_target,
        ddl_config,
        wdd_config,
        &videoFrameBuffer,
        _camConfig,
        WDD_WRITE_SIGNAL_FILE,
        WDD_WRITE_DANCE_FILE,
        WDD_VERBOSE,
        _dancePath);

    /*
    // Create camera instance
    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_GAIN = true!" << std::endl;

    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_EXPOSURE = true!" << std::endl;

    if (CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true) == false)
        std::cerr << "WARNING! Could not set CLEYE_AUTO_WHITEBALANCE = true!" << std::endl;

    */
    // TODO BEN: FIX

    printf("Start camera warmup..\n");
    bool warmupDone = false;
    unsigned int warmupFpsHit = 0;
    cv::Mat* framePtr = _camera->getFramePointer();
    cv::Mat frameTargetBc = frame_target;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (_running) {
        _camera->nextFrame();

        // save to global Frame Buffer
        videoFrameBuffer.addFrame(framePtr);
        cv::Mat frame_input_monochrome = *framePtr;

        // subsample
        cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
            0, 0, cv::INTER_AREA);

        //
        if (_visual) {
            drawArena(frameTargetBc);
            cv::imshow(_windowName, frameTargetBc);
        }

        if (!warmupDone) {
            _wdd->copyInitialFrame(frame_counter_global, false);
        } else {
            // save number of frames needed for camera warmup
            _wdd->copyInitialFrame(frame_counter_global, true);
            break;
        }
        // finally increase frame_input counter
        frame_counter_global++;

        //test fps
        if ((frame_counter_global % 100) == 0) {
            std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
            double fps = 100 / sec.count();
            printf("%1.f fps ..", fps);
            // TODO BEN: INCREASED ALLOWED OFFSET FROM 1 TO 5
            if (abs(WDD_FRAME_RATE - fps) < 5) {
                printf("\t [GOOD]\n");
                warmupDone = ++warmupFpsHit >= 3 ? true : false;
            } else {
                printf("\t [BAD]\n");
            }

            fflush(stdout);
            start = std::chrono::steady_clock::now();
        }
    }
    printf("Camera warmup done!\n\n\n");
    fflush(stdout);

    std::vector<double> bench_res;
    while (_running) {
        _camera->nextFrame();

        // save to global Frame Buffer
        videoFrameBuffer.addFrame(framePtr);
        cv::Mat frame_input_monochrome = *framePtr;

        // subsample
        cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
            0, 0, cv::INTER_AREA);

        // feed WDD with tar_frame
        _wdd->copyFrame(frame_counter_global);

        if (_visual && (frame_counter_global % 25 == 0)) {
            cv::cvtColor(frame_target, frame_visual, CV_GRAY2BGR);
            drawPosDDs(frame_visual);
            cv::resize(frame_visual, frame_visual, cv::Size2i(640, 480),
                0, 0, cv::INTER_LINEAR);
            drawArena(frame_visual);

            cv::imshow(_windowName, frame_visual);
            cv::waitKey(1);
        }

#ifdef WDD_DDL_DEBUG_FULL
        if (frame_counter_global >= WDD_FBUFFER_SIZE - 1)
            printf("Frame# %llu\t DD_SIGNALS_NUMBER: %d\n", WaggleDanceDetector::WDD_SIGNAL_FRAME_NR, DotDetectorLayer::DD_SIGNALS_NUMBER);

        // check exit condition
        if ((frame_counter_global - frame_counter_warmup) >= WDD_DDL_DEBUG_FULL_MAX_FRAME - 1) {
            std::cout << "************** WDD_DDL_DEBUG_FULL DONE! **************" << std::endl;
            printf("WDD_DDL_DEBUG_FULL captured %d frames.\n", WDD_DDL_DEBUG_FULL_MAX_FRAME);
            _running = false;
        }
#endif
        // finally increase frame_input counter
        frame_counter_global++;
        // benchmark output
        if ((frame_counter_global % 100) == 0) {
            std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
            bench_res.push_back(100 / sec.count());
            start = std::chrono::steady_clock::now();
        }

        if ((frame_counter_global % 500) == 0) {
            time_t now = time(0);
            struct tm* tstruct = localtime(&now);
            char buf[80];
            // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
            // for more information about date/time format
            strftime(buf, sizeof(buf), "%Y%m%d %X", tstruct);
            std::cout << "[" << buf << "] collected fps: ";
            double avg = 0;
            for (auto it = bench_res.begin(); it != bench_res.end(); ++it) {
                printf("%.1f ", *it);
                avg += *it;
            }
            printf("(avg: %.1f)\n", avg / bench_res.size());
            bench_res.clear();

            makeHeartBeatFile();
            fflush(stdout);
        }
    }
}
size_t ProcessingThread::CaptureThread()
{
    srand(time(nullptr));

    if (this->_setupModeOn)
        this->Setup();
    else
        this->Run();

    return 0;
}
}
