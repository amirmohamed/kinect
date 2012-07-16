cmake_minimum_required (VERSION 2.8)

project (oneshot)

find_library(OpenNI_LIBS OpenNI)
find_path(OpenNI_INCLUDES XnOpenNI.h PATHS /usr/include/ni)

#find_package (OpenCV REQUIRED)
include(FindPkgConfig)
pkg_check_modules(OpenCV REQUIRED opencv)

set(NITE_INCLUDE_DIR /home/xavier/dev/kinect/NITE-Bin-Dev-Linux-x64-v1.5.2.21/Include/)
include_directories( ${NITE_INCLUDE_DIR} )

include_directories(${OpenCV_INCLUDE_DIRS} ${OpenNI_INCLUDES}) #/usr/include/ni)

#message ("blbla ${OpenNI_INCLUDES}")

set (CMAKE_C_FLAGS "-g -std=gnu99")

add_executable (depthDisplay depthDisplay.cpp)
target_link_libraries(depthDisplay ${OpenCV_LIBRARIES} ${OpenNI_LIBS})
