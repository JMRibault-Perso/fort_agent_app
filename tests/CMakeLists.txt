add_executable(
    ${CMAKE_PROJECT_NAME}_test
    ${CMAKE_PROJECT_NAME}_test.cpp
)

target_link_libraries(
    ${CMAKE_PROJECT_NAME}_test
    PRIVATE
    ${CMAKE_PROJECT_NAME}_lib
    gtest_main
    boost_asio
    boost_circular_buffer
    spdlog
)

add_test(
    NAME
    ${CMAKE_PROJECT_NAME}_test
    COMMAND
    ${CMAKE_PROJECT_NAME}_test
)
