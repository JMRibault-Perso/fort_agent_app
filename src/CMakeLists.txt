###############################################################################
# src/CMakeLists.txt
###############################################################################

set(HEADER_PATH ${CMAKE_SOURCE_DIR}/include/${CMAKE_PROJECT_NAME})

set(HEADER_LIST
    ${HEADER_PATH}/coapHelpers.h
    ${HEADER_PATH}/coapPortTracker.h
    ${HEADER_PATH}/dbgTrace.h
    ${HEADER_PATH}/serialHandler.h
    ${HEADER_PATH}/slip.h
    ${HEADER_PATH}/spammyLogMsg.h
    ${HEADER_PATH}/uartCoapBridge.h
    ${HEADER_PATH}/uartCoapBridgeSingleton.h
    ${HEADER_PATH}/fort_agent.h

    ${HEADER_PATH}/jaus/JausBridge.h
    ${HEADER_PATH}/jaus/JausBridgeSingleton.h
    ${HEADER_PATH}/jaus/JausClient.h
    ${HEADER_PATH}/jaus/JausClientImpl.h
    ${HEADER_PATH}/jaus/vehicleStateMachine.h

    ${HEADER_PATH}/jaus/states/IVehicleState.h
    ${HEADER_PATH}/jaus/states/InitializeState.h
    ${HEADER_PATH}/jaus/states/StandbyState.h
    ${HEADER_PATH}/jaus/states/EmergencyState.h
    ${HEADER_PATH}/jaus/states/ShutdownState.h
    ${HEADER_PATH}/jaus/states/ReadyState.h
    ${HEADER_PATH}/jaus/states/ControlState.h

    ${HEADER_PATH}/uart/FORTJoystick/FORTJoystickHelpers.h
)

set(SOURCE_PATH ${CMAKE_SOURCE_DIR}/src)

set(SOURCE_LIST
    ${SOURCE_PATH}/coapHelpers.cpp
    ${SOURCE_PATH}/coapPortTracker.cpp
    ${SOURCE_PATH}/serialHandler.cpp
    ${SOURCE_PATH}/slip.cpp
    ${SOURCE_PATH}/uartCoapBridge.cpp
    ${SOURCE_PATH}/uartCoapBridgeSingleton.cpp
    ${SOURCE_PATH}/fort_agent.cpp
    ${SOURCE_PATH}/jaus/JausBridge.cpp
    ${SOURCE_PATH}/jaus/JausBridgeSingleton.cpp
    ${SOURCE_PATH}/jaus/JausClientImpl.cpp
    ${SOURCE_PATH}/uart/FORTJoystick/FORTJoystickHelpers.cpp
)

# ---------------------------------------------------------------------------
# TinyCBOR: build from source if not using system package
# ---------------------------------------------------------------------------
if(NOT USE_SYSTEM_TINYCBOR)
    add_library(tinycbor STATIC
        ${tinycbor_SOURCE_DIR}/src/cborencoder.c
        ${tinycbor_SOURCE_DIR}/src/cborencoder_close_container_checked.c
        ${tinycbor_SOURCE_DIR}/src/cborencoder_float.c
        ${tinycbor_SOURCE_DIR}/src/cborerrorstrings.c
        ${tinycbor_SOURCE_DIR}/src/cborparser.c
        ${tinycbor_SOURCE_DIR}/src/cborparser_dup_string.c
        ${tinycbor_SOURCE_DIR}/src/cborparser_float.c
        ${tinycbor_SOURCE_DIR}/src/cborpretty.c
        ${tinycbor_SOURCE_DIR}/src/cborpretty_stdio.c
        ${tinycbor_SOURCE_DIR}/src/cborvalidation.c
        ${tinycbor_SOURCE_DIR}/src/cbortojson.c
        ${tinycbor_SOURCE_DIR}/src/open_memstream.c
    )
    target_include_directories(tinycbor PUBLIC ${tinycbor_SOURCE_DIR}/src)
endif()

# ---------------------------------------------------------------------------
# Main library
# ---------------------------------------------------------------------------
add_library(${CMAKE_PROJECT_NAME}_lib ${SOURCE_LIST} ${HEADER_LIST})

target_compile_definitions(${CMAKE_PROJECT_NAME}_lib
    PUBLIC ${CMAKE_PROJECT_NAME}_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_compile_options(${CMAKE_PROJECT_NAME}_lib
    PUBLIC -fdata-sections -ffunction-sections
)

target_include_directories(${CMAKE_PROJECT_NAME}_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${Boost_INCLUDE_DIRS}
        $<$<NOT:$<BOOL:${USE_SYSTEM_TINYCBOR}>>:${tinycbor_SOURCE_DIR}/src>
        $<$<BOOL:${USE_SYSTEM_TINYCBOR}>:${TINYCBOR_INCLUDE_DIR}>
)

target_link_libraries(${CMAKE_PROJECT_NAME}_lib
    PRIVATE
        spdlog::spdlog
        fmt::fmt
        $<$<NOT:$<BOOL:${USE_SYSTEM_TINYCBOR}>>:tinycbor>
        $<$<BOOL:${USE_SYSTEM_TINYCBOR}>:${TINYCBOR_LIB}>
        OpenJAUS::core
        OpenJAUS::mobility
        OpenJAUS::base
)

# ---------------------------------------------------------------------------
# Force full static linking for OpenJAUS libraries on aarch64
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    foreach(lib IN ITEMS OpenJAUS::core OpenJAUS::mobility OpenJAUS::base)
        get_target_property(lib_path ${lib} IMPORTED_LOCATION)
        target_link_options(${CMAKE_PROJECT_NAME}_lib
            PRIVATE "-Wl,--whole-archive" "${lib_path}" "-Wl,--no-whole-archive"
        )
    endforeach()
endif()

###############################################################################
# Install
###############################################################################
install(TARGETS ${CMAKE_PROJECT_NAME}_lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# OpenJAUS configuration file
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/fort_agent_app.ojconf
        DESTINATION ${CMAKE_INSTALL_BINDIR})
###############################################################################