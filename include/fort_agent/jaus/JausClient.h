// JAUSClient.h
#pragma once

#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

class JAUSClient {
public:
    virtual bool discoverVehicle() = 0;
    virtual bool sendRequestControl() = 0;
    virtual bool hasControl() const = 0;
    virtual bool isHeartbeatAlive() const = 0;
    virtual void sendWrenchEffort(const frc_joystick_data_t& data) = 0;
    virtual ~JAUSClient() = default;
};