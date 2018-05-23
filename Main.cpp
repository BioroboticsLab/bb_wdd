#include "CLEyeCameraCapture.h"
#include "Camera.h"
#include "Config.h"
#include "DotDetectorLayer.h"
#include "InputVideoParameters.h"
#include "Util.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"
#include "WaggleDanceExport.h"
#include "opencv2/opencv.hpp"
#include <Guid.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <tclap/CmdLine.h>

using namespace wdd;

// save executable name
static char _NAME_OF_EXE[FILENAME_MAX];

// save full path to executable
static char _FULL_PATH_EXE[FILENAME_MAX];

static std::string GLOB_WDD_DANCE_OUTPUT_PATH;

void runTestMode(std::string videoFilename, double aux_dd_min_potential, int aux_wdd_signal_min_cluster_size, bool noGui);

void getNameOfExe(char* out, char* argv0)
{
    std::string argv0_str(argv0);
    std::string exeName;

    std::size_t found = argv0_str.find_last_of("\\");

    if (found == std::string::npos)

        exeName = argv0_str;
    else
        exeName = argv0_str.substr(found + 1);

    // check if .exe is at the end
    found = exeName.find_last_of(".exe");
    if (found == std::string::npos)
        exeName += ".exe";

    strcpy(out, exeName.c_str());
}
void getExeFullPath(char* out)
{
    strcpy(out, boost::dll::program_location().parent_path().c_str());
}

bool fileExists(const std::string& file_name)
{
    return boost::filesystem::is_regular_file(boost::filesystem::path(file_name));
}

bool dirExists(const char* dirPath)
{
    return boost::filesystem::is_directory(boost::filesystem::path(dirPath));
}

// saves loaded, modified camera configs
static std::vector<CamConf> camConfs;

// saves the next unique camId
static std::size_t nextUniqueCamID = 0;

// format of config file:
// <camId> <GUID> <Arena.p1> <Arena.p2> <Arena.p3> <Arena.p4>
void loadCamConfigFileReadLine(std::string line)
{
    const char* delimiter = " ";
    std::size_t pos = 0;

    std::size_t tokenNumber = 0;

    std::size_t camId = NULL;
    char guid_str[64];
    std::array<cv::Point2i, 4> arena;

    //copy & convert to char *
    char* string1 = strdup(line.c_str());

    // parse the line
    char* token = nullptr;
    token = strtok(string1, delimiter);

    int arenaPointNumber = 0;
    while (token != nullptr) {
        int px, py;

        // camId
        if (tokenNumber == 0) {
            camId = atoi(token);
        }
        // guid
        else if (tokenNumber == 1) {
            strcpy(guid_str, token);
        } else {
            switch (tokenNumber % 2) {
                // arena.pi.x
            case 0:
                px = atoi(token);
                break;
                // arena.pi.y
            case 1:
                py = atoi(token);
                arena[arenaPointNumber++] = cv::Point2i(px, py);
                break;
            }
        }

        tokenNumber++;
        token = strtok(NULL, delimiter);
    }
    free(string1);
    free(token);

    if (tokenNumber != 10)
        std::cerr << "Warning! cams.config file contains corrupted line with total tokenNumber: " << tokenNumber << std::endl;

    struct CamConf c;
    c.camId = camId;
    strcpy(c.guid_str, guid_str);
    c.arena = arena;
    c.configured = true;

    // save loaded CamConf  to global vector
    camConfs.push_back(c);

    // keep track of loaded camIds and alter nextUniqueCamID accordingly
    if (camId >= nextUniqueCamID)
        nextUniqueCamID = camId + 1;
}
static const std::string camConfPath = "cams.config";
void loadCamConfigFile()
{
    boost::filesystem::path path(_FULL_PATH_EXE);
    path /= camConfPath;

    if (!fileExists(path.c_str())) {
        // create empty file
        std::fstream f;
        f.open(path.c_str(), std::ios::out);
        f << std::flush;
        f.close();
    }

    std::string line;
    std::ifstream camconfigfile;
    camconfigfile.open(path.c_str());

    if (camconfigfile.is_open()) {
        while (getline(camconfigfile, line)) {
            loadCamConfigFileReadLine(line);
        }
        camconfigfile.close();
    } else {
        std::cerr << "Error! Can not open cams.config file!" << std::endl;
        exit(111);
    }
}
void saveCamConfigFile()
{
    boost::filesystem::path path(_FULL_PATH_EXE);
    path /= camConfPath;

    FILE* camConfFile_ptr = fopen(path.c_str(), "w+");

    for (auto it = camConfs.begin(); it != camConfs.end(); ++it) {
        if (it->configured) {
            fprintf(camConfFile_ptr, "%d %s ", it->camId, it->guid_str);
            for (unsigned i = 0; i < 4; i++)
                fprintf(camConfFile_ptr, "%d %d ", it->arena[i].x, it->arena[i].y);
            fprintf(camConfFile_ptr, "\n");
        }
    }
    fclose(camConfFile_ptr);
}

int main(int nargs, char** argv)
{
    // get full name of executable
    getNameOfExe(_NAME_OF_EXE, argv[0]);

    // get the full path to executable
    getExeFullPath(_FULL_PATH_EXE);

    char* version = "1.2.5";
    char* compiletime = __TIMESTAMP__;
    printf("WaggleDanceDetection Version %s - compiled at %s\n\n",
        version, compiletime);

    // define values potentially set by command line
    double dd_min_potential;
    int wdd_signal_min_cluster_size;
    bool autoStartUp;
    std::string videofile;
    bool noGui;
    std::string dancePath;
    try {
        // Define the command line object.
        TCLAP::CmdLine cmd("Command description message", ' ', version);

        // Define a value argument and add it to the command line.
        TCLAP::ValueArg<double> potArg("p", "potential", "Potential minimum value", false, 32888, "double");
        cmd.add(potArg);

        // Define a value argument and add it to the command line.
        TCLAP::ValueArg<int> cluArg("c", "cluster", "Cluster minimum size", false, 6, "int");
        cmd.add(cluArg);

        // Define a switch and add it to the command line.
        TCLAP::SwitchArg autoSwitch("a", "auto", "Selects automatically configured cam", false);
        cmd.add(autoSwitch);

#if defined(TEST_MODE_ON)
        // path to test video input file
        TCLAP::ValueArg<std::string> testVidArg("t", "video", "path to video file", true, "", "string");
        cmd.add(testVidArg);

        // path to output of dance detection file
        TCLAP::ValueArg<std::string> outputArg("o", "output", "path to result file", false, "", "string");
        cmd.add(outputArg);

        // switch to turn off GUI/Video output
        TCLAP::SwitchArg noGUISwitch("n", "no-gui", "disable gui (video output)", false);
        cmd.add(noGUISwitch);
#endif
        // Parse the args.
        cmd.parse(nargs, argv);

        // Get the value parsed by each arg.
        dd_min_potential = potArg.getValue();
        wdd_signal_min_cluster_size = cluArg.getValue();
        autoStartUp = autoSwitch.getValue();
        videofile = testVidArg.getValue();
        noGui = noGUISwitch.getValue();
        dancePath = outputArg.getValue();

        if (dancePath.size()) {
            char BUFF[FILENAME_MAX];
            strcpy(BUFF, dancePath.c_str());
            GLOB_WDD_DANCE_OUTPUT_PATH = std::string(BUFF);
        } else {
            char BUFF[FILENAME_MAX];
            strcpy(BUFF, _FULL_PATH_EXE);
            strcat(BUFF, "\\dance.txt");
            GLOB_WDD_DANCE_OUTPUT_PATH = std::string(BUFF);
        }
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }

    // call in test env
    if (videofile.size()) {
        runTestMode(videofile, dd_min_potential, wdd_signal_min_cluster_size, noGui);
        exit(0);
    }

    char videoFilename[FILENAME_MAX];
    // WaggleDanceExport initialization
    WaggleDanceExport::execRootExistChk();

    // Query for number of connected cameras
    size_t numCams = Camera::getNumCameras();

    if (numCams == 0) {
        printf("Error No PS3Eye cameras detected\n");
        exit(-1);
    }
    std::cout << numCams << " Cameras detected." << std::endl;

    std::vector<std::string> cameraIdentifiers;
    // Query & temporaly save unique camera uuid
    for (int i = 0; i < numCams; i++) {
        cameraIdentifiers.push_back(Camera::getIdentifier(i));
    }

    // Load & store cams.config file
    loadCamConfigFile();

    // prepare container for camIds
    std::vector<std::size_t> camIdsLaunch;

    // merge data from file and current connected cameras
    // compare guids connected to guids loaded from file
    // - if match, camera has camId and arena properties -> configured=true
    // - else it gets new camId -> configured=false
    for (int i = 0; i < numCams; i++) {
        bool foundMatch = false;

        const std::string identifier = Camera::getIdentifier(i);

        for (auto it = camConfs.begin(); it != camConfs.end(); ++it) {
            if (strcmp(identifier.c_str(), it->guid_str) == 0) {
                foundMatch = true;
                camIdsLaunch.push_back(it->camId);
                break;
            }
        }

        // no match found -> new config
        // find next camId
        if (!foundMatch) {
            struct CamConf c;
            c.camId = nextUniqueCamID++;
            camIdsLaunch.push_back(c.camId);
            strcpy(c.guid_str, identifier.c_str());
            c.arena[0] = cv::Point2i(0, 0);
            c.arena[1] = cv::Point2i(639, 0);
            c.arena[2] = cv::Point2i(639, 479);
            c.arena[3] = cv::Point2i(0, 479);
            c.configured = false;

            camConfs.push_back(c);
        }
    }

    // if autoStartUp flag, iterate camIdsLaunch and select first configured camId
    int camIdUserSelect = -1;
    if (autoStartUp) {
        for (std::size_t i = 0; i < camIdsLaunch.size(); i++) {
            std::size_t _camId = camIdsLaunch[i];
            CamConf* cc_ptr = nullptr;
            for (auto it = camConfs.begin(); it != camConfs.end(); ++it)
                if (it->camId == _camId)
                    if (it->configured) {
                        camIdUserSelect = _camId;
                        break;
                    }
        }

        if (camIdUserSelect < 0)
            std::cout << "\nWARNING! Could not autostart because no configrued camera present!\n\n";
    }

    if (camIdUserSelect < 0) {
        // for all camIds pushed to launch retrieve information and push to user prompt
        printf("CamID\tGUID\tconfigured?\n");
        printf("***********************************\n");

        for (std::size_t i = 0; i < camIdsLaunch.size(); i++) {
            std::size_t _camId = camIdsLaunch[i];
            CamConf* cc_ptr = nullptr;
            for (auto it = camConfs.begin(); it != camConfs.end(); ++it)
                if (it->camId == _camId)
                    cc_ptr = &(*it);

            if (cc_ptr != nullptr)
                printf("%d\t %s\t%s\n", cc_ptr->camId, cc_ptr->guid_str, cc_ptr->configured ? "true" : "false");
        }
        printf("\n\n");

        // retrieve users camId choice
        while (camIdUserSelect < 0) {
            std::string in;
            std::cout << " -> Please select camera id to start:" << std::endl;
            std::getline(std::cin, in);

            try {
                std::size_t i_dec = static_cast<std::size_t>(std::stoi(in, nullptr));
                for (auto it = camIdsLaunch.begin(); it != camIdsLaunch.end(); ++it) {
                    if (*it == i_dec) {
                        std::cout << " -> " << i_dec << " selected!" << std::endl;
                        camIdUserSelect = i_dec;
                    } else {
                        std::cout << i_dec << " unavailable!" << std::endl;
                    }
                }
            } catch (const std::invalid_argument& ia) {
                std::cerr << "Invalid argument: " << ia.what() << '\n';
            }
        }
    }

    // retrieve guid from camConfs according to camId
    std::unique_ptr<CLEyeCameraCapture> pCam;
    const std::string windowName = cameraIdentifiers[camIdUserSelect];
    for (auto it = camConfs.begin(); it != camConfs.end(); ++it) {
        if (it->camId == camIdUserSelect) {
            printf(windowName.c_str(), "WaggleDanceDetector - CamID: %d", camIdUserSelect);

            bool foundMatch = false;
            for (int i = 0; i < numCams; i++) {
                if (strcmp(cameraIdentifiers[i].c_str(), it->guid_str) == 0) {
                    foundMatch = true;
                    pCam = std::make_unique<CLEyeCameraCapture>(
                        windowName, cameraIdentifiers[i], camIdUserSelect, 320, 240, WDD_FRAME_RATE, *it,
                        dd_min_potential, wdd_signal_min_cluster_size);
                    break;
                }
            }
            std::cout << foundMatch << std::endl;
        }
    }

    printf("Starting WaggleDanceDetector - CamID: %d\n", camIdUserSelect);

    const CamConf* cc_ptr = pCam->getCamConfPtr();

    if (!cc_ptr->configured) {
        // run Setup mode first
        pCam->setSetupModeOn(true);
        pCam->StartCapture();

        pCam->setSetupModeOn(false);

        // update camConfs
        for (auto it = camConfs.begin(); it != camConfs.end(); ++it) {
            if (it->camId == cc_ptr->camId) {
                camConfs.erase(it);
                break;
            }
        }
        camConfs.push_back(*cc_ptr);
        saveCamConfigFile();
    }

    pCam->StartCapture();
    return 0;
}

void runTestMode(std::string videoFilename, double aux_DD_MIN_POTENTIAL, int aux_WDD_SIGNAL_MIN_CLUSTER_SIZE, bool noGui)
{

    std::cout << "************** Run started in test mode **************\n";
    if (!fileExists(videoFilename)) {
        std::cerr << "Error! Wrong video path!\n";
        exit(-201);
    }

    cv::VideoCapture capture(0);
    if (!capture.open(videoFilename)) {
        std::cerr << "Error! Video input stream broken - check openCV install "
                     "(https://help.ubuntu.com/community/OpenCV)\n";
        capture.release();
        exit(-202);
    }
    //
    //	Global: video configuration
    //
    //TODO
    int FRAME_RED_FAC = 4; //4 -> 1/16 -> 320 ?= x * 1/16 ->

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
    bool visual = !noGui;
    bool wdd_write_dance_file = true;
    bool wdd_write_signal_file = false;
    int wdd_verbose = 0;

    // prepare frame_counter
    unsigned long long frame_counter_global = 0;
    unsigned long long frame_counter_warmup = 0;

    InputVideoParameters vp(&capture);
    int FRAME_WIDTH = vp.getFrameWidth();
    int FRAME_HEIGHT = vp.getFrameHeight();
    int FRAME_RATE = WDD_FRAME_RATE;

    /*
	struct CamConf c;
	c.camId = nextUniqueCamID++;	
	strcpy_s(c.guid_str, "virtual-cam-config");
	c.arena[0] = cv::Point2i(0,0);
	c.arena[1] = cv::Point2i(320-1,0);
	c.arena[2] = cv::Point2i(320-1,240-1);
	c.arena[3] = cv::Point2i(0,240-1);
	// prepare videoFrameBuffer
	VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(FRAME_WIDTH, FRAME_HEIGHT), cv::Size(20,20), c);
    */
    // TODO BEN: FIX

    cv::Mat frame_input;
    cv::Mat frame_input_monochrome;
    cv::Mat frame_target;

    if (!noGui)
        cv::namedWindow("Video");

    /* prepare frame_counter */
    unsigned long long frame_counter = 0;

    /* prepare buffer to hold mono chromized input frame */
    frame_input_monochrome = cv::Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC1);

    /* prepare buffer to hold target frame */
    double resize_factor = pow(2.0, FRAME_RED_FAC);

    int frame_target_width = cvRound(FRAME_WIDTH / resize_factor);
    int frame_target_height = cvRound(FRAME_HEIGHT / resize_factor);

    std::cout << "Printing WaggleDanceDetector frame parameter:" << std::endl;
    printf("frame_height: %d\n", frame_target_height);
    printf("frame_width: %d\n", frame_target_width);
    printf("frame_rate: %d\n", FRAME_RATE);
    printf("frame_red_fac: %d\n", FRAME_RED_FAC);
    frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);

    /*
	* prepare DotDetectorLayer config vector
	*/
    std::vector<double> ddl_config;
    ddl_config.push_back(DD_FREQ_MIN);
    ddl_config.push_back(DD_FREQ_MAX);
    ddl_config.push_back(DD_FREQ_STEP);
    ddl_config.push_back(FRAME_RATE);
    ddl_config.push_back(FRAME_RED_FAC);
    ddl_config.push_back(DD_MIN_POTENTIAL);

    std::vector<cv::Point2i> dd_positions;
    for (int i = 0; i < frame_target_width; i++) {
        for (int j = 0; j < frame_target_height; j++) {
            // x (width), y(height)
            dd_positions.push_back(cv::Point2i(i, j));
        }
    }
    printf("Initialize with %d DotDetectors (DD_MIN_POTENTIAL=%.1f).\n",
        dd_positions.size(), DD_MIN_POTENTIAL);

    /*
	* prepare WaggleDanceDetector config vector
	*/
    std::vector<double> wdd_config;
    // Layer 2
    wdd_config.push_back(WDD_SIGNAL_DD_MAXDISTANCE);
    wdd_config.push_back(WDD_SIGNAL_MIN_CLUSTER_SIZE);
    // Layer 3
    wdd_config.push_back(WDD_DANCE_MAX_POSITION_DISTANCEE);
    wdd_config.push_back(WDD_DANCE_MAX_FRAME_GAP);
    wdd_config.push_back(WDD_DANCE_MIN_CONSFRAMES);

    // TODO BEN: FIX
    /*
    WaggleDanceDetector wdd(
		dd_positions,
		&frame_target,
		ddl_config,
		wdd_config,
		&videoFrameBuffer,
		c,
		wdd_write_signal_file,
		wdd_write_dance_file,
		wdd_verbose
		);
        */

    /*
	const std::map<std::size_t,cv::Point2d> * WDDSignalId2PointMap = wdd.getWDDSignalId2PointMap();
	const std::vector<DANCE> * WDDFinishedDances = wdd.getWDDFinishedDancesVec();
    */
    // TODO BEN: FIX

    const std::map<std::size_t, cv::Point2d> WDDDance2PointMap;

    int Cir_radius = 3;
    cv::Scalar Cir_color_yel = cv::Scalar(255, 255, 0);
    cv::Scalar Cir_color_gre = cv::Scalar(0, 255, 0);
    cv::Scalar Cir_color_som = cv::Scalar(0, 0, 255);
    //make the circle filled with value < 0
    int Cir_thikness = -1;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    std::vector<double> bench_res;
    //loop_bench_res_sing.reserve(dd_positions.size());
    //loop_bench_avg.reserve(1000);

    while (capture.read(frame_input)) {
        /*

		//convert BGR -> Gray
		cv::cvtColor(frame_input,frame_input_monochrome, CV_BGR2GRAY);

		// save to global Frame Buffer
		videoFrameBuffer.addFrame(&frame_input_monochrome);

		// subsample
		cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
			0, 0, cv::INTER_AREA);

		// feed WDD with tar_frame
		if(frame_counter < WDD_FBUFFER_SIZE-1)
		{
			wdd.copyInitialFrame(frame_counter, false);
		}
		else if (frame_counter == WDD_FBUFFER_SIZE-1)
		{
			wdd.copyInitialFrame(frame_counter, true);			
		}
		else 
		{
			wdd.copyFrame(frame_counter);
		}

		// output visually if enabled
		if(visual)
		{
			if(DotDetectorLayer::DD_SIGNALS_NUMBER>0)
			{
				for(auto it=DotDetectorLayer::DD_SIGNALS_IDs.begin(); it!= DotDetectorLayer::DD_SIGNALS_IDs.end(); ++it)
					cv::circle(frame_input, DotDetectorLayer::positions.at(*it)*std::pow(2, FRAME_RED_FAC), 2, Cir_color_som, Cir_thikness);
			}
			//check for WDD Signal
			if(wdd.isWDDSignal())
			{
				for(std::size_t i=0; i< wdd.getWDDSignalNumber(); i++)
				{
					cv::circle(frame_input, (*WDDSignalId2PointMap).find(i)->second*std::pow(2,FRAME_RED_FAC),
						Cir_radius, Cir_color_yel, Cir_thikness);
				}
			}
			bool waitLongerThanEver = false;
			if(wdd.isWDDDance())
			{

				for(auto it = WDDFinishedDances->begin(); it!=WDDFinishedDances->end(); ++it)
				{
					if( (*it).DANCE_FRAME_END >= frame_counter-10)
					{
						cv::circle(frame_input, (*it).positions[0]*std::pow(2,FRAME_RED_FAC),
							Cir_radius, Cir_color_gre, Cir_thikness);
						cv::line(frame_input, (*it).positions[0] * std::pow(2, FRAME_RED_FAC), (*it).positions[0] * std::pow(2, FRAME_RED_FAC) - (*it).orient_uvec *1000* std::pow(2, FRAME_RED_FAC), Cir_color_yel);
						cv::line(frame_input, (*it).positions[0] * std::pow(2, FRAME_RED_FAC), (*it).positions[0] * std::pow(2, FRAME_RED_FAC) + (*it).naive_orientation * 1000 * std::pow(2, FRAME_RED_FAC), Cir_color_gre);
						waitLongerThanEver = true;
					}
				}
			}

			cv::imshow("Video", frame_input);
			cv::waitKey(10);
			if (waitLongerThanEver)
				Sleep(1000);
		}
#ifdef WDD_DDL_DEBUG_FULL
		if(frame_counter >= WDD_FBUFFER_SIZE-1)
			printf("Frame# %llu\t DD_SIGNALS_NUMBER: %d\n", WaggleDanceDetector::WDD_SIGNAL_FRAME_NR, DotDetectorLayer::DD_SIGNALS_NUMBER);

		if(frame_counter >= WDD_DDL_DEBUG_FULL_MAX_FRAME-1)
		{
			std::cout<<"************** WDD_DDL_DEBUG_FULL DONE! **************"<<std::endl;
			printf("WDD_DDL_DEBUG_FULL captured %d frames.\n", WDD_DDL_DEBUG_FULL_MAX_FRAME);
			capture.release();
			exit(0);
		}
#endif
		// finally increase frame_input counter	
		frame_counter++;
		// benchmark output
		if((frame_counter % 100) == 0)
		{
			std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;

			//std::cout<<"fps: "<< 100/sec.count() <<"("<< cvRound(sec.count()*1000)<<"ms)"<<std::endl;

			bench_res.push_back(100/sec.count());

			start = std::chrono::steady_clock::now();
		}
    */
        // TODO BEN: FIX
    }

    capture.release();
}
