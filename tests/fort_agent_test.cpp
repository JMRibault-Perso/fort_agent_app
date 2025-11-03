#include <gtest/gtest.h>

#include <string>
#include <fort_agent/version.h>

TEST(FortAgentVersionTest, MatchesProjectVersionMacro) {
    EXPECT_EQ(getFortAgentVersion(), std::string(fort_agent_VERSION));
}
