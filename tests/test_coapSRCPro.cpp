#include <gtest/gtest.h>
#include <uart/FORTJoystick/coapSRCPro.h>

using namespace coapSRCPro;

TEST(CoapSRCProTest, BuildRadioMode) {
    uint16_t mid = 123;
    auto msg = getRadioMode(mid);

    // Basic sanity checks
    EXPECT_FALSE(msg.empty());
    // You can add more detailed checks here, e.g. CoAP header fields
}

TEST(CoapSRCProTest, BuildSMCUSafety) {
    uint16_t mid = 456;
    int smcuIndex = 0;
    auto msg = getSMCUSafety(smcuIndex, mid);

    EXPECT_FALSE(msg.empty());
}

TEST(CoapSRCProTest, BuildSerialNumber) {
    uint16_t mid = 789;
    auto msg = getSerialNumber(mid);

    EXPECT_FALSE(msg.empty());

    auto post = postSerialNumber(mid, "SN12345");
    EXPECT_FALSE(post.empty());
}

TEST(CoapSRCProTest, BuildDisplayMode) {
    uint16_t mid = 321;
    auto getMsg = getDisplayMode(mid);
    EXPECT_FALSE(getMsg.empty());

    auto postMsg = postDisplayMode(mid, 1);
    EXPECT_FALSE(postMsg.empty());
}

TEST(CoapSRCProTest, BuildFirmwareFileData) {
    uint16_t mid = 654;
    auto getMsg = getFirmwareFileData(mid, "firmware.bin");
    EXPECT_FALSE(getMsg.empty());

    std::vector<uint8_t> dummyData{0x01, 0x02, 0x03};
    auto postMsg = postFirmwareFileData(mid, "firmware.bin", dummyData);
    EXPECT_FALSE(postMsg.empty());
}

class ObservableEndpointsTest : public ::testing::Test {
protected:
    uint16_t mid = 100;
};

// Helper macro to reduce duplication
#define TEST_OBSERVABLE(Name) \
TEST_F(ObservableEndpointsTest, Name##_Get) { \
    auto msg = get##Name(mid, true, 0); \
    EXPECT_FALSE(msg.empty()); \
} \
TEST_F(ObservableEndpointsTest, Name##_Subscribe) { \
    auto msg = subscribe##Name(mid); \
    EXPECT_FALSE(msg.empty()); \
} \
TEST_F(ObservableEndpointsTest, Name##_Unsubscribe) { \
    auto msg = unsubscribe##Name(mid); \
    EXPECT_FALSE(msg.empty()); \
}

// Generate tests for each observable endpoint
TEST_OBSERVABLE(CombinedJoystickKeypad)
TEST_OBSERVABLE(CpuTempC)
TEST_OBSERVABLE(DeviceTempC)
TEST_OBSERVABLE(GaugeTempC)
TEST_OBSERVABLE(GyroTempC)
TEST_OBSERVABLE(BatteryStatus)

#undef TEST_OBSERVABLE
