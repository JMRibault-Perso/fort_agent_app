#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/EmergencyState.h>
#include <fort_agent/jaus/states/StandbyState.h>


#include <chrono>

/**
 * @brief Active control state where joystick data flows continuously to JAUS.
 *
 * The state monitors heartbeat health and control ownership.  A missed
 * heartbeat triggers @ref EmergencyState.  Control loss can be handled in the
 * future by returning to standby.
 */
class ReadyState : public IVehicleState {
public:
    ReadyState(JAUSClient& client) : 
        IVehicleState(), client(client), lastHeartbeatCheck(std::chrono::steady_clock::now()) 
        {}

    /** Vibrate motors and inform the driver when the vehicle is ready. */
    void enter() override {
        displayTextOnJoystick("Ready", "Joystick active");
        vibrateJoystick(true, true); // Vibrate both motors on entering ready state
    }

    /** Forward every joystick report to JAUS as a wrench effort setpoint. */
    void handleInput(const frc_combined_data_t& input) override {
        // Send joystick data to JAUS
        client.sendWrenchEffort(input.joystick_data);
        }

    /** Placeholder hook for JAUS responses that arrive while READY. */
    void handleResponse() override {
        // Really should not have anything but this is where we can handle heartbeat stuff
    }

    /**
     * @brief Monitor heartbeat cadence and control ownership.
     *
     * Runs each control loop tick to ensure the vehicle remains responsive and
     * keeps authority; loss of either triggers a state transition.
     */
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

    /** Transition to EmergencyState when the heartbeat check fails. */
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

    /** Lightweight bitmask helper for keypad input state. */
    bool isButtonPressed(uint16_t status, KeypadButton button) {
        return (status & static_cast<uint16_t>(button)) != 0;
    }
};