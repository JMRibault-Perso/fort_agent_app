// IVehicleState.h

#pragma once
#include <memory>
#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

class IVehicleState {
public:
    IVehicleState() = default;

    virtual void enter() = 0;
    virtual void handleInput(const frc_combined_data_t & input) = 0;
    virtual void handleResponse() = 0;
    virtual void update() = 0;
    virtual std::unique_ptr<IVehicleState> next() = 0;
    virtual ~IVehicleState() = default;
};

