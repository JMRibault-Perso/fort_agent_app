#include <fort_agent/fort_agent.h>
#include <gtest/gtest.h>

namespace {
    TEST(FortAgentTest, RememberToTest) {
        EXPECT_EQ(getFortAgentVersion(), std::string("0.0.1"));
    }
}
