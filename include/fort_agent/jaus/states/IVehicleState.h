#pragma once
#include <memory>
#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

/**
 * @brief Interface implemented by each high-level vehicle state.
 *
 * The JAUS bridge pushes joystick input and asynchronous JAUS responses through
 * the state machine.  Each state decides what to do with that input and may
 * request a transition by returning a new state from next().
 */
class IVehicleState {
public:
    IVehicleState() = default;

    /** Called once when the state becomes active. */
    virtual void enter() = 0;
    /**
     * Process joystick/keypad input pulled from the FORT controller.
     * Implementations may initiate JAUS requests or update internal flags.
     */
    virtual void handleInput(const frc_combined_data_t & input) = 0;
    /** React to the latest JAUS response queued by the client. */
    virtual void handleResponse() = 0;
    /** Periodic tick invoked from the bridge service loop. */
    virtual void update() = 0;
    /**
     * Optionally return the next state. Returning nullptr keeps the current
     * state active.
     */
    virtual std::unique_ptr<IVehicleState> next() = 0;
    virtual ~IVehicleState() = default;
};

