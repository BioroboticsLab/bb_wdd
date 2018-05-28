#include "CLEyeCameraCapture.h"
#include "Config.h"
#include "DotDetectorLayer.h"
#include "Util.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"

#include <boost/filesystem.hpp>
#include <cstdio>

namespace wdd {

_MouseInteraction _Mouse;

WaggleDanceDetector* WDD_p = nullptr;

void CLEyeCameraCapture::findCornerPointNear(cv::Point2i p) const
{
    // we actually have a 4 point arena to work with
    if (_CC.arena.size() != 4)
        printf("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _CC.arena.size());

    // check if there is already a corner marked as hovered
    if (_Mouse.cornerHovered > -1) {
        // revalidate distance
        if (cv::norm(_CC.arena[_Mouse.cornerHovered] - cv::Point2i(p)) < 25) {
            // nothing to do, no GUI update needed
            return;
        } else {
            // unset corner marked as hovered
            _Mouse.cornerHovered = -1;
            // done
            return;
        }
    }

    //find the corner the mouse points at
    // assertion Mouse.cornerHover = -1;
    for (std::size_t i = 0; i < _CC.arena.size(); i++) {
        if (cv::norm(_CC.arena[i] - cv::Point2i(p)) < 25) {
            _Mouse.cornerHovered = i;
            break;
        }
    }
}
void CLEyeCameraCapture::onMouseInput(int evnt, int x, int y, int flags)
{
    cv::Point2i p(x, y);
    _Mouse.lastPosition = p;

    switch (evnt) {
    case CV_EVENT_MOUSEMOVE:
        // if a corner is selected, move the edge position
        if (_Mouse.cornerSelected > -1) {
            _CC.arena[_Mouse.cornerSelected] = p;
        } else {
            // hover over corner point should trigger visual feedback
            findCornerPointNear(p);
        }
        break;

    case CV_EVENT_LBUTTONDOWN:
        // implement toggle: if corner was already selected, deselect it
        if (_Mouse.cornerSelected > -1)
            _Mouse.cornerSelected = -1;
        // else check if corner is hovered (=in range) and possibly select it
        else if (_Mouse.cornerHovered > -1)
            _Mouse.cornerSelected = _Mouse.cornerHovered;
        break;
    }
}
void CLEyeCameraCapture::onMouseInput(int evnt, int x, int y, int flags, void* userData)
{
    CLEyeCameraCapture* self = reinterpret_cast<CLEyeCameraCapture*>(userData);
    self->onMouseInput(evnt, x, y, flags);
}
CLEyeCameraCapture::CLEyeCameraCapture(std::string windowName, std::string cameraGUID, size_t cameraIdx, size_t width, size_t height, float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size, const std::string& dancePath)
    : _windowName(windowName)
    , _cameraGUID(cameraGUID)
    , _cameraIdx(cameraIdx)
    , _camera(std::make_unique<Camera>(cameraIdx, fps, width, height))
    , _fps(fps)
    , _running(false)
    , _visual(true)
    , _FRAME_WIDTH(width)
    , _FRAME_HEIGHT(height)
    , aux_DD_MIN_POTENTIAL(dd_min_potential)
    , aux_WDD_SIGNAL_MIN_CLUSTER_SIZE(wdd_signal_min_cluster_size)
    , _CC(CC)
    , _dancePath(dancePath)
{
    //_setupModeOn = !_CC.configured;
    _setupModeOn = false;
}
bool CLEyeCameraCapture::StartCapture()
{
    _running = true;

    cv::namedWindow(_windowName);

    CaptureThread();

    return true;
}
void CLEyeCameraCapture::setVisual(bool visual)
{
    _visual = visual;
}
void CLEyeCameraCapture::setSetupModeOn(bool setupMode)
{
    _setupModeOn = setupMode;
}
const CamConf* CLEyeCameraCapture::getCamConfPtr()
{
    return &_CC;
}
void CLEyeCameraCapture::Setup()
{
    std::cout << "Setup" << std::endl;

    // listen for mouse interaction
    cv::setMouseCallback(_windowName, onMouseInput, this);
    // id der Ecke, die mit der Maus angehovert wurde
    _Mouse.cornerHovered = -1;
    // id der Ecke, die mit der Maus angeklickt wurde
    _Mouse.cornerSelected = -1;

    int w, h;
    IplImage* pCapImage;

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
    _CC.configured = true;
}
void CLEyeCameraCapture::drawArena(cv::Mat& frame)
{
    // we actually have a 4 point arena to work with
    if (_CC.arena.size() != 4) {
        printf("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _CC.arena.size());
    }
    // setupModeOn -> expect 640x480
    if (_setupModeOn) {
        for (std::size_t i = 0; i < _CC.arena.size(); i++) {
            cv::line(frame, _CC.arena[i], _CC.arena[(i + 1) % _CC.arena.size()], CV_RGB(0, 255, 0), 2, CV_AA);
            cv::circle(frame, _CC.arena[i], 5, CV_RGB(0, 255, 0), 2, CV_AA);

            if (_Mouse.cornerSelected > -1)
                cv::circle(frame, _CC.arena[_Mouse.cornerSelected], 5, CV_RGB(255, 255, 255), 2, CV_AA);
        }
    }
    // else from 640x480 -> 180x120
    else {
        for (std::size_t i = 0; i < _CC.arena.size(); i++) {
            cv::line(frame, _CC.arena[i], _CC.arena[(i + 1) % _CC.arena.size()], CV_RGB(0, 255, 0), 2, CV_AA);
        }
    }
}
void CLEyeCameraCapture::drawPosDDs(cv::Mat& frame)
{
    for (auto it = DotDetectorLayer::DD_SIGNALS_IDs.begin(); it != DotDetectorLayer::DD_SIGNALS_IDs.end(); ++it) {
        cv::circle(frame, DotDetectorLayer::positions.at(*it), 1, CV_RGB(0, 255, 0));
    }
}

char* hbf_extension = ".wtchdg";
void CLEyeCameraCapture::makeHeartBeatFile()
{
    boost::filesystem::path path(getExeFullPath());
    path /= getNameOfExe();
    std::string pathStr(path.string());
    pathStr += hbf_extension;

    FILE* pFile = fopen(pathStr.c_str(), "w");

    if (pFile != nullptr)
        fclose(pFile);
    else
        std::cerr << "ERROR! Could not create heartbeat file: " << path << std::endl;
}

//Adoption from stackoverflow
//http://stackoverflow.com/questions/13080515/rotatedrect-roi-in-opencv
//decide whether point p is in the ROI.
//The ROI is a RotatableBox whose 4 corners are stored in pt[]
bool CLEyeCameraCapture::pointIsInArena(cv::Point p)
{
    return true;
    double result[4];
    for (int i = 0; i < 4; i++) {
        result[i] = computeProduct(p, _CC.arena_lowRes[i], _CC.arena_lowRes[(i + 1) % 4]);
        // all have to be 1, exit on first negative encounter
        if (result[i] < 0)
            return false;
    }
    if (result[0] + result[1] + result[2] + result[3] == 4)
        return true;
    else
        return false;
}
//Adoption from stackoverflow
//http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
//Use the sign of the determinant of vectors (AB,AM), where M(X,Y) is the query point:
double CLEyeCameraCapture::computeProduct(const cv::Point p, const cv::Point2i a, const cv::Point2i b)
{
    double x = ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x));

    return (x >= 0) ? 1 : -1;
}
void CLEyeCameraCapture::Run()
{
    std::cout << "Run" << std::endl;

    _camera->setCaptureProps(_fps, _FRAME_WIDTH, _FRAME_HEIGHT);

    //
    //	Global: video configuration
    //
    int FRAME_RED_FAC = 1;

    //
    //	Layer 1: DotDetector Configuration
    //
    int DD_FREQ_MIN = 11;
    int DD_FREQ_MAX = 17;
    double DD_FREQ_STEP = 1;
    double DD_MIN_POTENTIAL = aux_DD_MIN_POTENTIAL;

    //
    //	Layer 2: Waggle SIGNAL Configuration
    //
    double WDD_SIGNAL_DD_MAXDISTANCE = 2.3;
    int WDD_SIGNAL_MIN_CLUSTER_SIZE = aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;

    //
    //	Layer 3: Waggle Dance Configuration
    //
    double WDD_DANCE_MAX_POSITION_DISTANCEE = sqrt(2);
    int WDD_DANCE_MAX_FRAME_GAP = 3;
    int WDD_DANCE_MIN_CONSFRAMES = 20;

    //
    //	Develop: Waggle Dance Configuration
    //
    bool visual = true;
    bool wdd_write_dance_file = false;
    bool wdd_write_signal_file = false;
    int wdd_verbose = 0;

    // prepare frame_counter
    unsigned long long frame_counter_global = 0;
    unsigned long long frame_counter_warmup = 0;

    // prepare videoFrameBuffer
    VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(_FRAME_WIDTH, _FRAME_HEIGHT), cv::Size(20, 20), _CC);

    // prepare buffer to hold mono chromized input frame
    //cv::Mat frame_input_monochrome = cv::Mat(_FRAME_HEIGHT, _FRAME_WIDTH, CV_8UC1);

    // prepare buffer for possible output
    cv::Mat frame_visual;
    IplImage frame_visual_ipl = frame_visual;

    // prepare buffer to hold target frame
    double resize_factor = pow(2.0, FRAME_RED_FAC);

    int frame_target_width = cvRound(_FRAME_WIDTH / resize_factor);
    int frame_target_height = cvRound(_FRAME_HEIGHT / resize_factor);
    cv::Mat frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);

    std::cout << "Printing WaggleDanceDetector frame parameter:" << std::endl;
    printf("frame_height: %d\n", frame_target_height);
    printf("frame_width: %d\n", frame_target_width);
    printf("frame_rate: %d\n", WDD_FRAME_RATE);
    printf("frame_red_fac: %d\n", FRAME_RED_FAC);

    //
    // prepare DotDetectorLayer config vector
    //
    std::vector<double> ddl_config;
    ddl_config.push_back(DD_FREQ_MIN);
    ddl_config.push_back(DD_FREQ_MAX);
    ddl_config.push_back(DD_FREQ_STEP);
    ddl_config.push_back(WDD_FRAME_RATE);
    ddl_config.push_back(FRAME_RED_FAC);
    ddl_config.push_back(DD_MIN_POTENTIAL);

    //
    // select DotDetectors according to Arena
    //

    // assert: Setup() before Run() -> arena configured, 640x480 -> 180x120
    std::cout << _CC.arena[0] << std::endl;
    for (int i = 0; i < 4; i++) {
        _CC.arena_lowRes[i].x = static_cast<int>(std::floor(_CC.arena[i].x * 0.25));
        _CC.arena_lowRes[i].y = static_cast<int>(std::floor(_CC.arena[i].y * 0.25));
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

    //
    // prepare WaggleDanceDetector config vector
    //
    std::vector<double> wdd_config;
    // Layer 2
    wdd_config.push_back(WDD_SIGNAL_DD_MAXDISTANCE);
    wdd_config.push_back(WDD_SIGNAL_MIN_CLUSTER_SIZE);
    // Layer 3
    wdd_config.push_back(WDD_DANCE_MAX_POSITION_DISTANCEE);
    wdd_config.push_back(WDD_DANCE_MAX_FRAME_GAP);
    wdd_config.push_back(WDD_DANCE_MIN_CONSFRAMES);

    WaggleDanceDetector wdd(
        dd_positions,
        &frame_target,
        ddl_config,
        wdd_config,
        &videoFrameBuffer,
        _CC,
        wdd_write_signal_file,
        wdd_write_dance_file,
        wdd_verbose,
        _dancePath);

    WDD_p = &wdd;

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
    bool WARMUP_DONE = false;
    unsigned int WARMUP_FPS_HIT = 0;
    cv::Mat* framePtr = _camera->getFramePointer();
    cv::Mat frame_target_bc = frame_target;
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
            drawArena(frame_target_bc);
            cv::imshow(_windowName, frame_target_bc);
        }

        if (!WARMUP_DONE) {
            wdd.copyInitialFrame(frame_counter_global, false);
        } else {
            // save number of frames needed for camera warmup
            frame_counter_warmup = frame_counter_global;
            wdd.copyInitialFrame(frame_counter_global, true);
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
                WARMUP_DONE = ++WARMUP_FPS_HIT >= 3 ? true : false;
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
        wdd.copyFrame(frame_counter_global);

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
size_t CLEyeCameraCapture::CaptureThread()
{
    srand(time(nullptr));

    if (this->_setupModeOn)
        this->Setup();
    else
        this->Run();

    return 0;
}
}
