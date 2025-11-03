###############################################################################
# Project metadata
###############################################################################
cmake_minimum_required(VERSION 3.11)

project(
    fort_agent
    VERSION 1.0.2
    DESCRIPTION "Serial UDP Proxy for RTOS devices to communicate over IP"
    LANGUAGES C CXX
)

# Detect architecture and set OpenJAUS SDK paths
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(OJ_ARCH "build-aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(OJ_ARCH "build")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(OJ_SDK_ROOT "$ENV{HOME}/openjaus-customers-iop-sdk/org.openjaus.iop-sdk.cpp")
# set(OJ_SDK_ROOT "${OPENJAUS_SDK_ROOT}")
set(OJ_SDK_LIB_DIR "${OJ_SDK_ROOT}/${OJ_ARCH}")

# Include OpenJAUS CMake module
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(OpenJAUS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

###############################################################################
# Dependencies
###############################################################################

# Option to control whether to use FetchContent (for devs) or system packages (for Yocto)
option(USE_FETCHCONTENT "Download dependencies with FetchContent" ON)

if(USE_FETCHCONTENT)
    include(FetchContent)

    # googletest (only needed for tests)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
    )
    FetchContent_MakeAvailable(googletest)

    # fmt
    FetchContent_Declare(
        fmtlib
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.1.1
    )
    FetchContent_MakeAvailable(fmtlib)

    # spdlog
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.9.2
    )
    FetchContent_MakeAvailable(spdlog)

    # Boost (dev builds)
    FetchContent_Declare(
        boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.83.0
    )
    FetchContent_MakeAvailable(boost)

    # TinyCBOR (dev builds)
    FetchContent_Declare(
        tinycbor
        GIT_REPOSITORY https://github.com/intel/tinycbor.git
        GIT_TAG v0.6.0
    )
    FetchContent_MakeAvailable(tinycbor)

    set(USE_SYSTEM_TINYCBOR OFF)

else()
    # Yocto/system packages
    find_package(Boost 1.84 EXACT REQUIRED COMPONENTS program_options)
    find_package(fmt REQUIRED)
    find_package(spdlog REQUIRED)

    # Try CMake config first
    find_package(tinycbor CONFIG)
    if(tinycbor_FOUND)
        set(USE_SYSTEM_TINYCBOR ON)
    else()
        # Fallback: look for headers and library manually
        find_path(TINYCBOR_INCLUDE_DIR cbor.h)
        find_library(TINYCBOR_LIB cbor)
        if(NOT TINYCBOR_INCLUDE_DIR OR NOT TINYCBOR_LIB)
            message(FATAL_ERROR "System TinyCBOR not found")
        endif()
        set(USE_SYSTEM_TINYCBOR ON)
    endif()
endif()

###############################################################################
# Add Library, Application, and Tests
###############################################################################
add_subdirectory(src)
add_subdirectory(apps)

# Only build tests if enabled
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

###############################################################################
# Install rules
###############################################################################
# Library and headers are installed in src/CMakeLists.txt
# Application is installed in apps/CMakeLists.txt