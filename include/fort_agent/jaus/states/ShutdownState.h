#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/StandbyState.h>

class ShutdownState : public IVehicleState {
public:
    ShutdownState(JAUSClient& client)
        : IVehicleState(), client(client), rDownPressed(false) {}

    void enter() override {
        displayTextOnJoystick("Shutdown", "Press 1 to restart");
    }

    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            nextState = std::make_unique<StandbyState>(client);
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false;
        }
    }

    void update() override {}

    void handleResponse() override {}

    std::unique_ptr<IVehicleState> next() override {
        return std::move(nextState);
    }

private:
    JAUSClient& client;
    bool rDownPressed;
    std::unique_ptr<IVehicleState> nextState;

    bool isRDown(uint16_t status) {
        constexpr uint16_t R_DOWN_MASK = 1 << 7;
        return status & R_DOWN_MASK;
    }
};