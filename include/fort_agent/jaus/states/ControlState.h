#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/StandbyState.h>

class ControlState : public IVehicleState {
public:
    ControlState(JAUSClient& client, UartCoapBridge& display) : 
        IVehicleState(display), client(client), controlRequested(false), controlGranted(false), rDownPressed(false) {}

    void enter() override {
        display.postUserDisplayTest("Control Vehicle", "Press R-Down to Standby");
    }

    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            if (!controlRequested) {
                controlRequested = client.sendRequestControl();
                display.postUserDisplayTest("Requesting", "Control...");
                // This will trigger a response later to indicate that control is granted
            } else if (client.hasControl()) {
                controlGranted = true;
                // display.postUserDisplayTest("Control Granted", "Press R-Down");
            }
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false;
        }
    }

    void update() override {}

    std::unique_ptr<IVehicleState> next() override {
        return controlGranted ? std::make_unique<StandbyState>(client, display) : nullptr;
    }

private:
    JAUSClient& client;
    bool controlRequested;
    bool controlGranted;
    bool rDownPressed;

    bool isRDown(uint16_t status) {
        constexpr uint16_t R_DOWN_MASK = 1 << 7;
        return status & R_DOWN_MASK;
    }
};