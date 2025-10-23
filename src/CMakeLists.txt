###############################################################################
# Headers
###############################################################################
set(
    HEADER_PATH
    ${${CMAKE_PROJECT_NAME}_SOURCE_DIR}/include/${CMAKE_PROJECT_NAME}
)

set(
    HEADER_LIST
    ${HEADER_PATH}/coapHelpers.h
    ${HEADER_PATH}/coapPortTracker.h
    ${HEADER_PATH}/dbgTrace.h
    ${HEADER_PATH}/serialHandler.h
    ${HEADER_PATH}/slip.h
    ${HEADER_PATH}/spammyLogMsg.h
    ${HEADER_PATH}/uartCoapBridge.h
    ${HEADER_PATH}/coapJausBridge.h
    ${HEADER_PATH}/fort_agent.h
)

###############################################################################
# Sources
###############################################################################
set(
    SOURCE_PATH
    ${${CMAKE_PROJECT_NAME}_SOURCE_DIR}/src
)

set(
    SOURCE_LIST
    ${SOURCE_PATH}/coapHelpers.cpp
    ${SOURCE_PATH}/coapPortTracker.cpp
    ${SOURCE_PATH}/serialHandler.cpp
    ${SOURCE_PATH}/slip.cpp
    ${SOURCE_PATH}/uartCoapBridge.cpp
    ${SOURCE_PATH}/coapJausBridge.cpp
    ${SOURCE_PATH}/fort_agent.cpp
)

###############################################################################
# Build & Link
###############################################################################
add_library(
    ${CMAKE_PROJECT_NAME}_lib
    ${SOURCE_LIST}
    ${HEADER_LIST}
)

target_compile_definitions(
    ${CMAKE_PROJECT_NAME}_lib
    PUBLIC
    ${CMAKE_PROJECT_NAME}_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_compile_options(
    ${CMAKE_PROJECT_NAME}_lib
    PUBLIC
    -fdata-sections
    -ffunction-sections
)

target_include_directories(
    ${CMAKE_PROJECT_NAME}_lib
    PUBLIC
    ../include
)

target_link_libraries(
    ${CMAKE_PROJECT_NAME}_lib
    PRIVATE
    spdlog
    boost_asio
    boost_circular_buffer
    boost_crc
    -static
)
