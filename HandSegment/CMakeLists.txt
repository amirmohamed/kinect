cmake_minimum_required (VERSION 2.8)

project (HandSegment)

find_library(OpenNI_LIBS OpenNI)
find_path(OpenNI_INCLUDES XnOpenNI.h PATHS /usr/include/ni)
find_library(XnVNite_LIBS XnVNite_1_5_2)
find_path(XnVNite_INCLUDES XnVSessionManager.h PATHS /usr/include/nite)

include(FindPkgConfig)
pkg_check_modules(OpenCV REQUIRED opencv)

include_directories(${OpenCV_INCLUDE_DIRS}  ${XnVNite_INCLUDES} ${OpenNI_INCLUDES}) #/usr/include/ni(te)

set (CMAKE_C_FLAGS "-g -c -msse3 -O2 -DNDEBUG")

add_executable (handSegment main.cpp)
target_link_libraries(handSegment ${OpenCV_LIBRARIES} ${XnVNite_LIBS} ${OpenNI_LIBS})

ADD_CUSTOM_COMMAND(TARGET handSegment
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy handSegment ../)
