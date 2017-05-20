cmake_minimum_required(VERSION 3.5.1)

project(SensorTests)
include(CTest)

set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")

include(DownloadProject.cmake)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
 
include_directories(mocks/include)
include_directories(mocks/include/arduino-mock)
include_directories(fakes)
 
add_executable(runTests tools_tests.cpp serial_reader_tests.cpp ../tools.cpp mocks/src/ArduinoMockAll.cc fakes/WString.cpp)
target_link_libraries(runTests gtest gmock_main pthread)

add_test(sensortests runTests)
