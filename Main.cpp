#include "Camera.h"
#include "Config.h"
#include "DotDetectorLayer.h"
#include "ProcessingThread.h"
#include "Util.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"
#include "WaggleDanceExport.h"
#include "opencv2/opencv.hpp"
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <tclap/CmdLine.h>

using namespace wdd;

// saves loaded, modified camera configs
static std::vector<CamConf> camConfs;

// saves the next unique camId
static std::size_t nextUniqueCamID = 0;

// format of config file:
// <camId> <GUID> <Arena.p1> <Arena.p2> <Arena.p3> <Arena.p4>
void loadCamConfigFileReadLine(std::string line)
{
    const char* delimiter = " ";

    std::size_t tokenNumber = 0;

    std::size_t camId = 0;
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
        token = strtok(nullptr, delimiter);
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
void loadCamConfigFile()
{
    boost::filesystem::path path(getExeFullPath());
    path /= CAM_CONF_PATH;

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
    boost::filesystem::path path(getExeFullPath());
    path /= CAM_CONF_PATH;

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
    const std::string version = "1.3.0";
    const std::string compiletime = __TIMESTAMP__;
    printf("WaggleDanceDetection Version %s - compiled at %s\n\n",
        version.c_str(), compiletime.c_str());

    // define values potentially set by command line
    double dd_min_potential;
    int wdd_signal_min_cluster_size;
    bool autoStartUp;
    std::string videofile;
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
#endif
        // Parse the args.
        cmd.parse(nargs, argv);

        // Get the value parsed by each arg.
        dd_min_potential = potArg.getValue();
        wdd_signal_min_cluster_size = cluArg.getValue();
        autoStartUp = autoSwitch.getValue();
        videofile = testVidArg.getValue();
        dancePath = outputArg.getValue();

        if (dancePath.empty()) {
            boost::filesystem::path path(getExeFullPath());
            dancePath = path.string();
        }
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    }

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
                bool found = false;
                for (auto it = camIdsLaunch.begin(); it != camIdsLaunch.end(); ++it) {
                    if (*it == i_dec) {
                        std::cout << " -> " << i_dec << " selected!" << std::endl;
                        camIdUserSelect = i_dec;
                        found = true;
                    }
                }
                if (!found) {
                    std::cout << i_dec << " unavailable!" << std::endl;
                }
            } catch (const std::invalid_argument& ia) {
                std::cerr << "Invalid argument: " << ia.what() << '\n';
            }
        }
    }

    // retrieve guid from camConfs according to camId
    std::unique_ptr<ProcessingThread> pCam;
    const std::string windowName = cameraIdentifiers[camIdUserSelect];
    for (auto it = camConfs.begin(); it != camConfs.end(); ++it) {
        if (it->camId == camIdUserSelect) {
            printf(windowName.c_str(), "WaggleDanceDetector - CamID: %d", camIdUserSelect);

            bool foundMatch = false;
            for (int i = 0; i < numCams; i++) {
                if (strcmp(cameraIdentifiers[i].c_str(), it->guid_str) == 0) {
                    foundMatch = true;
                    pCam = std::make_unique<ProcessingThread>(
                        windowName, cameraIdentifiers[i], camIdUserSelect, 320, 240, WDD_FRAME_RATE, *it,
                        dd_min_potential, wdd_signal_min_cluster_size, dancePath);
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
