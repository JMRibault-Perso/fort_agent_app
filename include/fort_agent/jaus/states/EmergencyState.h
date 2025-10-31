#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>

class EmergencyState : public IVehicleState {
public:
    EmergencyState(JAUSClient& client, UartCoapBridge& display)
        : IVehicleState(display), client(client) {}

    void enter() override {
        display.postUserDisplayTest("EMERGENCY", "Vehicle disabled");
        // Optionally send a JAUS emergency message or disable command
    }

    void handleInput(const frc_combined_data_t&) override {
        // Ignore input in emergency
    }

    void update() override {
        // Could monitor heartbeat or wait for manual reset
    }

    std::unique_ptr<IVehicleState> next() override {
        return nullptr; // Terminal state unless reset externally
    }

private:
    JAUSClient& client;
};