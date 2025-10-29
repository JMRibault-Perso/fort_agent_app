###############################################################################
# apps/CMakeLists.txt
###############################################################################

add_executable(${CMAKE_PROJECT_NAME}_app
    ${CMAKE_PROJECT_NAME}_app.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME}_app
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${CMAKE_PROJECT_NAME}_app
    PRIVATE
        ${CMAKE_PROJECT_NAME}_lib
        Boost::program_options   # only the app needs this
        spdlog::spdlog
        fmt::fmt
)

target_link_options(${CMAKE_PROJECT_NAME}_app
    PRIVATE "-Wl,--gc-sections"
)

install(TARGETS ${CMAKE_PROJECT_NAME}_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)