cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

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

set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

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
    "ProcessingThread.cpp"
    "ProcessingThread.h"
    "cvblob.cpp"
    "cvlabel.cpp"
    "cvblob.h"
    "Camera.cpp"
    "Camera.h"
    "Util.cpp"
    "Util.h"
    "Config.h"
)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope")

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-reorder -Wno-format")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")
endif()

message(${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${OpenCV_LIBS} ${ARMADILLO_LIBRARIES})
