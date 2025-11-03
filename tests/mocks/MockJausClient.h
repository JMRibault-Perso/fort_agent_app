#pragma once

#include <gmock/gmock.h>

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

class MockJAUSClient : public JAUSClient {
public:
    MOCK_METHOD(void, initializeJAUS, (), (override));
    MOCK_METHOD(bool, discoverVehicle, (), (override));
    MOCK_METHOD(bool, sendRequestControl, (), (override));
    MOCK_METHOD(bool, isRequestPending, (), (const, override));
    MOCK_METHOD(bool, hasControl, (), (const, override));
    MOCK_METHOD(bool, sendRequestResume, (), (override));
    MOCK_METHOD(bool, queryStatus, (), (override));
    MOCK_METHOD(bool, hasReadyState, (), (const, override));
    MOCK_METHOD(bool, isHeartbeatAlive, (), (const, override));
    MOCK_METHOD(void, sendWrenchEffort, (const frc_joystick_data_t& data), (override));
    MOCK_METHOD(std::string, getComponentName, (), (override));
};
