// ReadyState.h
#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/EmergencyState.h>
#include <fort_agent/jaus/states/StandbyState.h>


#include <chrono>

class ReadyState : public IVehicleState {
public:
    ReadyState(JAUSClient& client) : 
        IVehicleState(), client(client), lastHeartbeatCheck(std::chrono::steady_clock::now()) 
        {}

    void enter() override {
        displayTextOnJoystick("Ready", "Joystick active");
        vibrateJoystick(true, true); // Vibrate both motors on entering ready state
    }

    void handleInput(const frc_combined_data_t& input) override {
        // Send joystick data to JAUS
        client.sendWrenchEffort(input.joystick_data);
        }

    void handleResponse() override {
        // Really should not have anything but this is where we can handle heartbeat stuff
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
            hasControl = false;
        }
    }

    std::unique_ptr<IVehicleState> next() override {
        if (emergencyTriggered) {
            return std::make_unique<EmergencyState>(client);
        }

        if (!hasControl) {
            // Transition to StandbyState 
            //return std::make_unique<StandbyState>(client);
        }
        return nullptr;
    }

private:
    JAUSClient& client;
    bool emergencyTriggered = false;
    bool hasControl = true;
    std::chrono::steady_clock::time_point lastHeartbeatCheck;

    bool isButtonPressed(uint16_t status, KeypadButton button) {
        return (status & static_cast<uint16_t>(button)) != 0;
    }
};