// ReadyState.h
#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/EmergencyState.h>

#include <chrono>

class ReadyState : public IVehicleState {
public:
    ReadyState(JAUSClient& client, UartCoapBridge& display) : 
        IVehicleState(display), client(client), lastHeartbeatCheck(std::chrono::steady_clock::now()) 
        {}

    void enter() override {
        display.postUserDisplayTest("Ready", "Joystick active");
    }

    void handleInput(const frc_combined_data_t& input) override {
        // Send joystick data to JAUS
        client.sendWrenchEffort(input.joystick_data);
        }
    

    void update() override {
        auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeatCheck > std::chrono::seconds(1)) {
            lastHeartbeatCheck = now;
            if (!client.isHeartbeatAlive()) {
                emergencyTriggered = true;
            }
        }

        if (!client.hasControl()) {
            emergencyTriggered = true;
        }
    }

    std::unique_ptr<IVehicleState> next() override {
        if (emergencyTriggered) {
            return std::make_unique<EmergencyState>(client, display);
        }
        return nullptr;
    }

private:
    JAUSClient& client;
    bool emergencyTriggered = false;
    std::chrono::steady_clock::time_point lastHeartbeatCheck;

    bool isButtonPressed(uint16_t status, KeypadButton button) {
        return (status & static_cast<uint16_t>(button)) != 0;
    }
};