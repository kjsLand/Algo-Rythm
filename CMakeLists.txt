cmake_minimum_required(VERSION 3.22)

project(AlgoRythm VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ALGO_BUILD_APP    "Build the desktop app"     ON)
option(ALGO_BUILD_CLI    "Build the algo-cli tool"   ON)
option(ALGO_BUILD_TESTS  "Build the unit tests"      ON)
option(ALGO_BUILD_PLUGIN "Build the audio plugin"    OFF)

include(cmake/Dependencies.cmake)
include(cmake/Warnings.cmake)
include(cmake/EmbedData.cmake)

add_subdirectory(core)

if (ALGO_BUILD_APP)
    add_subdirectory(plugin)
endif()

if (ALGO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
