#pragma once

#define WDD_FBUFFER_SIZE 32
#define WDD_FREQ_NUMBER 7
#define WDD_FRAME_RATE 102
#define VFB_MAX_FRAME_HISTORY 600

#if !defined(ARMA_NO_DEBUG)
  #define ARMA_NO_DEBUG
#endif

#if !defined(WDD_EXTRACT_ORIENT)
	#define WDD_EXTRACT_ORIENT
#endif

/*
#if !defined(WDD_DDL_DEBUG_FULL)
	#define WDD_DDL_DEBUG_FULL
	#define WDD_DDL_DEBUG_FULL_MAX_FRAME 40
	#define WDD_DDL_DEBUG_FULL_MAX_DDS 10
	#define WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS 10
#endif
	*/

#include "targetver.h"

#include <chrono>
#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <cmath>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#define NOMINMAX
//#include <tchar.h>
#include "atlstr.h"
#include <windows.h>
#include "Shlwapi.h"

#include <array>
#include <map>
#include <limits>
#include <vector>

#include <armadillo>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
