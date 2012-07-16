cmake_minimum_required (VERSION 2.8)

project (LinuxMagExample)

find_library(OpenNI_LIBS OpenNI)
find_path(OpenNI_INCLUDES XnOpenNI.h PATHS /usr/include/ni)

#find_package (OpenCV REQUIRED)
include(FindPkgConfig)
pkg_check_modules(OpenCV REQUIRED opencv)


include_directories(${OpenCV_INCLUDE_DIRS} ${OpenNI_INCLUDES}) #/usr/include/ni)

#message ("blbla ${OpenNI_INCLUDES}")

set (CMAKE_C_FLAGS "-g -std=gnu99")

add_executable (main main.c)
target_link_libraries(main ${OpenCV_LIBRARIES} ${OpenNI_LIBS})
