// JAUSClient.h
#pragma once

#include <string>
#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

class JAUSClient {
public:
    virtual void initializeJAUS() = 0;
    virtual bool discoverVehicle() = 0;
    virtual bool sendRequestControl() = 0;
    virtual bool isRequestPending() const = 0;
    virtual bool hasControl() const = 0;
    virtual bool sendRequestResume() = 0;
    virtual bool queryStatus() = 0;
    virtual bool hasReadyState() const = 0;
    virtual bool isHeartbeatAlive() const = 0;
    virtual void sendWrenchEffort(const frc_joystick_data_t& data) = 0;
    virtual std::string getComponentName() = 0;
    virtual ~JAUSClient() = default;
};