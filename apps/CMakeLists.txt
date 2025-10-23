add_executable(
    ${CMAKE_PROJECT_NAME}_app
    ${CMAKE_PROJECT_NAME}_app.cpp
)

target_link_libraries(
    ${CMAKE_PROJECT_NAME}_app
    PRIVATE
    ${CMAKE_PROJECT_NAME}_lib
    spdlog
    boost_asio
    boost_program_options
    boost_circular_buffer
)

target_link_options(
    ${CMAKE_PROJECT_NAME}_app
    PRIVATE
    "-Wl,--gc-sections"
)