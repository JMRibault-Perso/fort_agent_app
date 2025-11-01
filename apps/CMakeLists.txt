###############################################################################
# apps/CMakeLists.txt
###############################################################################

set(APP_TARGET fort_agent_app)

# ---------------------------------------------------------------------------
# Source
# ---------------------------------------------------------------------------
add_executable(${APP_TARGET}
    fort_agent_app.cpp
)

# Only need your own project headers here; OpenJAUS includes come via imported targets
target_include_directories(${APP_TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(${APP_TARGET}
    PRIVATE
        ${CMAKE_PROJECT_NAME}_lib
        Boost::program_options
        spdlog::spdlog
        fmt::fmt
        OpenJAUS::core
        OpenJAUS::mobility
        OpenJAUS::base
)

# ---------------------------------------------------------------------------
# Link options
# ---------------------------------------------------------------------------
target_link_options(${APP_TARGET}
    PRIVATE "-Wl,--gc-sections"
)

# Force full static linking for OpenJAUS libraries on aarch64
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    foreach(lib IN ITEMS OpenJAUS::core OpenJAUS::mobility OpenJAUS::base)
        get_target_property(lib_path ${lib} IMPORTED_LOCATION)
        target_link_options(${APP_TARGET}
            PRIVATE "-Wl,--whole-archive" "${lib_path}" "-Wl,--no-whole-archive"
        )
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Config file handling
# ---------------------------------------------------------------------------
set(CONFIG_DIR ${CMAKE_SOURCE_DIR}/config)
set(CONFIG_DEST ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(copy_config_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CONFIG_DIR}/fort-agent.conf
        ${CONFIG_DEST}/fort-agent.conf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CONFIG_DIR}/fort_agent_app.ojconf
        ${CONFIG_DEST}/fort_agent_app.ojconf
    COMMENT "Copying config files into build tree for debugging"
)

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ${CONFIG_DIR}/fort-agent.conf
    ${CONFIG_DIR}/fort_agent_app.ojconf
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)