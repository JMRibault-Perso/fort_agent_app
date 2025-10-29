###############################################################################
# apps/CMakeLists.txt
###############################################################################

# Build the application
add_executable(
    ${CMAKE_PROJECT_NAME}_app
    ${CMAKE_PROJECT_NAME}_app.cpp
)

# Link against your internal library and real external libs
target_link_libraries(
    ${CMAKE_PROJECT_NAME}_app
    PRIVATE
        ${CMAKE_PROJECT_NAME}_lib
        spdlog
        boost_asio
        boost_program_options
        boost_circular_buffer
)

# Optional linker flags
target_link_options(
    ${CMAKE_PROJECT_NAME}_app
    PRIVATE
    "-Wl,--gc-sections"
)

# Ensure include paths are available
target_include_directories(
    ${CMAKE_PROJECT_NAME}_app
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)