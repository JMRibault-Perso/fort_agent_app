#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/ControlState.h>

class InitializeState : public IVehicleState {
public:
    InitializeState(JAUSClient& client, UartCoapBridge& display) : 
        IVehicleState(display), client(client), vehicleFound(false), rDownPressed(false) {}

    void enter() override {
        display.postUserDisplayTest("Searching", "Press R-Down");
    }

    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            vehicleFound = client.discoverVehicle();
            if (vehicleFound) {
                display.postUserDisplayTest("Vehicle found", "Press R-Down");
            } else {
                display.postUserDisplayTest("No vehicle", "Try again");
            }
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false; // reset for next edge
        }
    }

    void update() override {}

    std::unique_ptr<IVehicleState> next() override {
        return vehicleFound ? std::make_unique<ControlState>(client, display) : nullptr;
    }

private:
    JAUSClient& client;
    bool vehicleFound;
    bool rDownPressed;

    bool isRDown(uint16_t status) {
        constexpr uint16_t R_DOWN_MASK = 1 << 7;
        return status & R_DOWN_MASK;
    }
};