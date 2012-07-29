cmake_minimum_required (VERSION 2.8)

project (HandSegment)

find_library(OpenNI_LIBS OpenNI)
find_path(OpenNI_INCLUDES XnOpenNI.h PATHS /usr/include/ni)
find_library(XnVNite_LIBS XnVNite_1_5_2)
find_path(XnVNite_INCLUDES XnVSessionManager.h PATHS /usr/include/nite)
set(BOOST_INCLUDE_DIR /home/xavier/lib/boost_1_49_0)
include_directories(${BOOST_INCLUDE_DIR})

include(FindPkgConfig)
pkg_check_modules(OpenCV REQUIRED opencv)

find_package(Boost REQUIRED system)
find_package(Threads)

include_directories(${OpenCV_INCLUDE_DIRS} ${XnVNite_INCLUDES} ${OpenNI_INCLUDES}) #/usr/include/ni(te)

set (CMAKE_C_FLAGS "-g -c -msse3 -O2 -DNDEBUG")

add_executable (fingerSegment main.cpp)
target_link_libraries(fingerSegment ${OpenCV_LIBRARIES} ${XnVNite_LIBS} ${OpenNI_LIBS} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_CUSTOM_COMMAND(TARGET fingerSegment
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy fingerSegment ../)
