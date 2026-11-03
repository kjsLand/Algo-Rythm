cmake_minimum_required(VERSION 3.22)

project(AlgoRythm VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ALGO_BUILD_APP    "Build the desktop app"     ON)
option(ALGO_BUILD_CLI    "Build the algo-cli tool"   ON)
option(ALGO_BUILD_TESTS  "Build the unit tests"      ON)
option(ALGO_BUILD_PLUGIN "Build the audio plugin"    OFF)
option(ALGO_BUILD_TOOLS  "Build the developer tools" ON)

# enable_testing comes first, so the CLI folder can add its smoke tests.
if (ALGO_BUILD_TESTS)
    enable_testing()
endif()

include(cmake/Dependencies.cmake)
include(cmake/Warnings.cmake)
include(cmake/EmbedData.cmake)

add_subdirectory(core)

if (ALGO_BUILD_CLI)
    add_subdirectory(cli)
endif()

if (ALGO_BUILD_APP)
    add_subdirectory(plugin)
endif()

if (ALGO_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if (ALGO_BUILD_TESTS)
    add_subdirectory(tests)
endif()
