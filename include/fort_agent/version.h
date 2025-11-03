#pragma once

#include <string>

/**
 * @brief Retrieve the runtime version string for the fort_agent project.
 *
 * The string is populated from the project version defined in CMake and
 * compiled into the binary via the fort_agent_VERSION macro.
 */
std::string getFortAgentVersion();
