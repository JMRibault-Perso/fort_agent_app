#include <gtest/gtest.h>

#include <string>

std::string getFortAgentVersion();

TEST(FortAgentVersionTest, MatchesProjectVersionMacro) {
    EXPECT_EQ(getFortAgentVersion(), std::string(fort_agent_VERSION));
}
