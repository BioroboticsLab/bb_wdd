#pragma once

#define WDD_FBUFFER_SIZE 32
#define WDD_FREQ_NUMBER 7
#define WDD_FRAME_RATE 102

#if !defined(ARMA_NO_DEBUG)
  #define ARMA_NO_DEBUG
#endif

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
#include <tchar.h>
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
