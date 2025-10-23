###############################################################################
# Project metadata
###############################################################################
cmake_minimum_required(
    VERSION
    3.11
)

project(
    fort_agent
    VERSION 1.0.2
    DESCRIPTION "Serial UDP Proxy for RTOS devices to communicate over IP"
    LANGUAGES CXX
)

set(
    CMAKE_CXX_STANDARD
    17
)

###############################################################################
# Top Level Only
###############################################################################
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

###############################################################################
# Dependencies
###############################################################################
include(
    FetchContent
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(
    googletest
)

FetchContent_Declare(
    boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.78.0
)
FetchContent_MakeAvailable(
    boost
)

FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1)
FetchContent_MakeAvailable(
    fmtlib
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.9.2)
FetchContent_MakeAvailable(
    spdlog
)

###############################################################################
# Add Library, Application, and Tests
###############################################################################
add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(tests)
