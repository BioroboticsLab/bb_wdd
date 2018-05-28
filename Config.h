#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define TEST_MODE_ON

#define WDD_LAYER2_MAX_POS_DDS 200
#define WDD_FBUFFER_SIZE 32
#define WDD_FREQ_NUMBER 7

#if defined(TEST_MODE_ON)
#define WDD_FRAME_RATE 102
#else
#define WDD_FRAME_RATE 102
#endif

#define VFB_MAX_FRAME_HISTORY 600

#if !defined(ARMA_NO_DEBUG)
#define ARMA_NO_DEBUG
#endif

/*
#if !defined(WDD_DDL_DEBUG_FULL)
	#define WDD_DDL_DEBUG_FULL
	#define WDD_DDL_DEBUG_FULL_MAX_FRAME 40
	#define WDD_DDL_DEBUG_FULL_MAX_DDS 10
	#define WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS 10
#endif
	*/

#include <cmath>
#include <string>

static const std::string CAM_CONF_PATH = "cams.config";
static const std::string HBF_EXTENSION = ".wtchdg";
static const std::string VFB_ROOT_PATH = "fullframes";
static const std::string DANCE_FILE_NAME = "dance.txt";
static const std::string SIGNAL_FILE_NAME = "signal.txt";

//	Global: video configuration
static const int FRAME_RED_FAC = 1;

//	Layer 1: DotDetector Configuration
static const float DD_DETECTION_FACTOR = 200;
static const int DD_FREQ_MIN = 11;
static const int DD_FREQ_MAX = 17;
static const double DD_FREQ_STEP = 1;

//	Layer 2: Waggle SIGNAL Configuration
static const double WDD_SIGNAL_DD_MAXDISTANCE = 2.3;

//	Layer 3: Waggle Dance Configuration
static const double WDD_DANCE_MAX_POSITION_DISTANCEE = sqrt(2);
static const int WDD_DANCE_MAX_FRAME_GAP = 3;
static const int WDD_DANCE_MIN_CONSFRAMES = 20;

//	Develop: Waggle Dance Configuration
static const bool VISUAL = true;
static const bool WDD_WRITE_DANCE_FILE = false;
static const bool WDD_WRITE_SIGNAL_FILE = false;
static const int WDD_VERBOSE = 0;
