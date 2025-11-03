// VehicleStateMachine.h
#pragma once
#include <memory>
#include <fort_agent/jaus/states/IVehicleState.h>

class VehicleStateMachine {
public:
    VehicleStateMachine(std::unique_ptr<IVehicleState> initialState)
        : currentState(std::move(initialState)) {
        currentState->enter();
    }

    void handleInput(const frc_combined_data_t& msg) {
        currentState->handleInput(msg);
        transitionIfNeeded();
    }

    void handleResponse(void) {
        currentState->handleResponse();
        transitionIfNeeded();
    }

    void update() {
        currentState->update();
        transitionIfNeeded();
    }

private:
    std::unique_ptr<IVehicleState> currentState;

    void transitionIfNeeded() {
        auto next = currentState->next();
        if (next) {
            currentState = std::move(next);
            currentState->enter();
        }
    }
};
