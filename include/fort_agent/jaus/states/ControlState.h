#pragma once

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/states/IVehicleState.h>
#include <fort_agent/jaus/states/StandbyState.h>

/**
 * @brief Handles the transition from discovery to control negotiation.
 *
 * The user must request control via the R-Down button.  Once JAUS confirms
 * control ownership the machine advances to @ref StandbyState to await
 * activation.
 */
class ControlState : public IVehicleState {
public:
    ControlState(JAUSClient& client) : 
        IVehicleState(), client(client), controlRequested(false), controlGranted(false), rDownPressed(false) {}

    /** Display control instructions when the state activates. */
    void enter() override {
        displayTextOnJoystick("Control Vehicle", "Press 1 to enter Standby");
    }

    /**
     * On the first R-Down press a JAUS RequestControl command is sent.  The
     * response is processed asynchronously by handleResponse().
     */
    void handleInput(const frc_combined_data_t& input) override {
        if (isRDown(input.keypad_data.buttonStatus) && !rDownPressed) {
            rDownPressed = true;
            if (!controlRequested) {
                controlRequested = client.sendRequestControl();
                displayTextOnJoystick("Requesting control", "...");
                // This will trigger a response later to indicate that control is granted
            } // else we have to wait
        } else if (!isRDown(input.keypad_data.buttonStatus)) {
            rDownPressed = false;
        }
    }

    void handleResponse() override {
        if (client.hasControl()) {
            controlGranted = true;
        } else {
            // let ask again?
            displayTextOnJoystick("Requesting Control", "Retry...");
            client.sendRequestControl();
        }
    }
    
    void update() override {}

    /** Advance to StandbyState when JAUS grants control authority. */
    std::unique_ptr<IVehicleState> next() override {
        return controlGranted ? std::make_unique<StandbyState>(client) : nullptr;
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