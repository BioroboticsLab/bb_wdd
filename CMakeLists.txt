cmake_minimum_required(VERSION 2.8)

project(bb_waggledancedetector)

include(FindOpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else(OPENMP_FOUND)
    message("ERROR: OpenMP could not be found.")
endif(OPENMP_FOUND)

find_package(Armadillo REQUIRED)
include_directories(${ARMADILLO_INCLUDE_DIRS})

find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
    "Main.cpp"
    "DotDetector.cpp"
    "DotDetector.h"
    "DotDetectorLayer.cpp"
    "DotDetectorLayer.h"
    "WaggleDanceDetector.cpp"
    "WaggleDanceDetector.h"
    "WaggleDanceExport.cpp"
    "WaggleDanceExport.h"
    "WaggleDanceOrientator.cpp"
    "WaggleDanceOrientator.h"
    "VideoFrameBuffer.h"
    "VideoFrameBuffer.cpp"
    "DotDetectorMatrix.cpp"
    "DotDetectorMatrix.h"
    "InputVideoParameters.cpp"
    "InputVideoParameters.h"
    "TypeToString.h"
    "TypeToString.cpp"
    "cvblob.cpp"
    "cvlabel.cpp"
    "cvblob.h"
    "Camera.cpp"
    "Camera.h"
    "Util.cpp"
    "Util.h"
    "Config.h"
)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${ARMADILLO_LIBRARIES})
