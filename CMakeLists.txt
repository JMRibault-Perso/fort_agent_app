###############################################################################
# Project metadata
###############################################################################
cmake_minimum_required(VERSION 3.11)

project(
    fort_agent
    VERSION 1.0.2
    DESCRIPTION "Serial UDP Proxy for RTOS devices to communicate over IP"
    LANGUAGES CXX
)

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

    # Use system Boost for standalone builds (>=1.70 works, 1.78 on your dev box)
    # find_package(Boost 1.70 REQUIRED COMPONENTS program_options)
    FetchContent_Declare(
        boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.78.0
    )
    FetchContent_MakeAvailable(boost)

else()
    # Yocto/system packages: enforce Boost 1.84
    find_package(Boost 1.84 EXACT REQUIRED COMPONENTS program_options)
    find_package(fmt REQUIRED)
    find_package(spdlog REQUIRED)
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