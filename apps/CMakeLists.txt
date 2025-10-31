###############################################################################
# apps/CMakeLists.txt
###############################################################################

set(APP_TARGET fort_agent_app)

add_executable(${APP_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/fort_agent_app.cpp
)

# Public headers are already exported by the library;
# here we just need the top-level include path for the app.
target_include_directories(${APP_TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

find_library(OJ_JAUS_LIB
    NAMES openjaus
    PATHS ${OJ_SDK_LIB_DIR}/openjaus
    NO_DEFAULT_PATH
)

# Find OpenJAUS core and mobility libraries
find_library(OJ_CORE_LIB
    NAMES openjaus-core_v1_1
    PATHS ${OJ_SDK_LIB_DIR}/core-v1.1
    NO_DEFAULT_PATH
)

find_library(OJ_MOB_LIB
    NAMES openjaus-mobility_v1_1
    PATHS ${OJ_SDK_LIB_DIR}/mob-v1.1
    NO_DEFAULT_PATH
)

# Fail early if not found

if(NOT OJ_JAUS_LIB)
    message(FATAL_ERROR "Could not find openjaus in ${OJ_SDK_LIB_DIR}/openjaus")
endif()


if(NOT OJ_CORE_LIB)
    message(FATAL_ERROR "Could not find openjaus-core_v1_1 in ${OJ_SDK_LIB_DIR}/core-v1.1")
endif()

if(NOT OJ_MOB_LIB)
    message(FATAL_ERROR "Could not find openjaus-mobility_v1_1 in ${OJ_SDK_LIB_DIR}/mob-v1.1")
endif()

target_link_libraries(${APP_TARGET}
    PRIVATE
        ${CMAKE_PROJECT_NAME}_lib
        Boost::program_options   # only the app needs this
        spdlog::spdlog
        fmt::fmt
        # Link OpenJAUS libraries
        ${OJ_CORE_LIB}
        ${OJ_MOB_LIB}
        ${OJ_JAUS_LIB}
)

target_link_options(${APP_TARGET}
    PRIVATE "-Wl,--gc-sections"
)

# Associate config file with target (optional, for tooling)
list(APPEND fort_agent_app_CONFIG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/fort_agent_app.ojconf)


# Explicit install rule: always install as /usr/bin/fort_agent_app
install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# Install config file next to executable
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/fort_agent_app.ojconf
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/fort-agent.conf
        DESTINATION ${CMAKE_INSTALL_BINDIR})