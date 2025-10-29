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
    ${HEADER_PATH}/coapJausBridge.h
    ${HEADER_PATH}/fort_agent.h
)

set(SOURCE_PATH ${CMAKE_SOURCE_DIR}/src)

set(SOURCE_LIST
    ${SOURCE_PATH}/coapHelpers.cpp
    ${SOURCE_PATH}/coapPortTracker.cpp
    ${SOURCE_PATH}/serialHandler.cpp
    ${SOURCE_PATH}/slip.cpp
    ${SOURCE_PATH}/uartCoapBridge.cpp
    ${SOURCE_PATH}/coapJausBridge.cpp
    ${SOURCE_PATH}/fort_agent.cpp
)

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
)

# Link against external libraries
target_link_libraries(${CMAKE_PROJECT_NAME}_lib
    PRIVATE
        spdlog::spdlog
        fmt::fmt
        #boost_asio
        #boost_circular_buffer
        #boost_crc
)

###############################################################################
# Install
###############################################################################
install(TARGETS ${CMAKE_PROJECT_NAME}_lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})