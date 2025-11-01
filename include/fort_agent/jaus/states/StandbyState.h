#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/ReadyState.h>

class StandbyState : public IVehicleState {
public:
    StandbyState(JAUSClient& client, UartCoapBridge& display) : 
        IVehicleState(display), client(client), resumeRequested(false), readyStateGranted(false), rDownPressed(false) {}

    void enter() override {
        display.postUserDisplayTest("Vehicle on Standby", "Press R-Down to Resume");
    }

    // TODO, some of those action should be triggered by JAUS messages/state managment instead of keypad input
    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            if (!resumeRequested) {
                resumeRequested = client.sendRequestResume();
                display.postUserDisplayTest("Requesting", "Resume state...");
                // This will trigger a response later to indicate that control is granted
            } else if (client.hasReadyState()) {
                readyStateGranted = true;
            } else {
                // status is not reported automatically, so we query it
                client.queryStatus();
            }
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false;
        }
    }

    void update() override {}

    std::unique_ptr<IVehicleState> next() override {
        return readyStateGranted ? std::make_unique<ReadyState>(client, display) : nullptr;
    }

private:
    JAUSClient& client;
    bool resumeRequested;
    bool readyStateGranted;
    bool rDownPressed;

    bool isRDown(uint16_t status) {
        constexpr uint16_t R_DOWN_MASK = 1 << 7;
        return status & R_DOWN_MASK;
    }
};