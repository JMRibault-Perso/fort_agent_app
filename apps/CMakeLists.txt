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

target_link_libraries(${APP_TARGET}
    PRIVATE
        ${CMAKE_PROJECT_NAME}_lib
        Boost::program_options   # only the app needs this
        spdlog::spdlog
        fmt::fmt
)

target_link_options(${APP_TARGET}
    PRIVATE "-Wl,--gc-sections"
)

# Explicit install rule: always install as /usr/bin/fort_agent_app
install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)