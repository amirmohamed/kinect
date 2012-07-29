cmake_minimum_required (VERSION 2.8)

project (AsioTest)

find_package(Boost REQUIRED system)
FIND_PACKAGE(Threads)

set (CMAKE_C_FLAGS "-g")

add_executable (Client KinectClient.cpp)
target_link_libraries(Client ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_CUSTOM_COMMAND(TARGET Client
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy Client ../)
