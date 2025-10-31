#pragma once

//OpenJAUS
#include <openjaus/Components/Base.h>
#include <openjaus/mobility_v1_1/GlobalPoseSensor.h>
#include <openjaus/mobility_v1_1/PrimitiveDriver.h>
#include <openjaus/system/Application.h>

#include <fort_agent/jaus/JausClient.h>

class JAUSClientImpl : public JAUSClient {
public:
    JAUSClientImpl();
    bool discoverVehicle() override;
    bool sendRequestControl() override;
    bool hasControl() const override;
    bool isHeartbeatAlive() const override;
    void sendWrenchEffort(const frc_joystick_data_t& data) override;

private:
    openjaus::components::Base component;
    openjaus::transport::Address serverAddress;

    bool controlGranted = false;
    bool reportGlobalPoseSubscribed = false;
    uint32_t reportGlobalPoseSubscriptionId = 0;
    bool reportWrenchEffortSubscribed = false;
    uint32_t reportWrenchEffortSubscriptionId = 0;

};