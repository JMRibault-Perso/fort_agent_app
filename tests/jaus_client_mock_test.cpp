#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mocks/MockJausClient.h"

using ::testing::Return;

TEST(MockJausClientTest, SupportsExpectationsOnControlFlow) {
    MockJAUSClient mock;

    EXPECT_CALL(mock, initializeJAUS()).Times(1);
    EXPECT_CALL(mock, discoverVehicle()).WillOnce(Return(true));
    EXPECT_CALL(mock, sendRequestControl()).WillOnce(Return(true));
    EXPECT_CALL(mock, hasControl()).WillOnce(Return(true));

    mock.initializeJAUS();
    EXPECT_TRUE(mock.discoverVehicle());
    EXPECT_TRUE(mock.sendRequestControl());
    EXPECT_TRUE(mock.hasControl());
}

TEST(MockJausClientTest, HandlesWrenchEffortForwarding) {
    MockJAUSClient mock;
    frc_joystick_data_t joystickData{};
    joystickData.leftXAxis.data = 1000;
    joystickData.leftXAxis.dataOK = true;

    EXPECT_CALL(mock, sendWrenchEffort(::testing::Ref(joystickData))).Times(1);
    mock.sendWrenchEffort(joystickData);
}
