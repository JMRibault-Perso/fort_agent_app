#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/ReadyState.h>

class StandbyState : public IVehicleState {
public:
    StandbyState(JAUSClient& client, UartCoapBridge& display) : 
        IVehicleState(display), client(client), controlRequested(false), controlGranted(false), rDownPressed(false) {}

    void enter() override {
        display.postUserDisplayTest("Standby", "Press R-Down");
    }

    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            if (!controlRequested) {
                controlRequested = client.sendRequestControl();
                display.postUserDisplayTest("Requesting", "Control...");
            } else if (client.hasControl()) {
                controlGranted = true;
                display.postUserDisplayTest("Control OK", "Press R-Down");
            }
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false;
        }
    }

    void update() override {}

    std::unique_ptr<IVehicleState> next() override {
        return controlGranted ? std::make_unique<ReadyState>(client, display) : nullptr;
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