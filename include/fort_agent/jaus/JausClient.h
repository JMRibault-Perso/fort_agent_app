#pragma once

#include <string>

#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

/**
 * @brief Abstract transport between the FORT agent and a JAUS-capable vehicle.
 *
 * Concrete implementations expose discovery, control negotiation, heartbeat
 * monitoring, and wrench-effort streaming.  The bridge and state machine rely
 * solely on this interface so it can be substituted in tests.
 */
class JAUSClient {
public:
    virtual ~JAUSClient() = default;

    /** Initialise the underlying OpenJAUS component and register callbacks. */
    virtual void initializeJAUS() = 0;
    /**
     * Discover a remote vehicle that exposes the JAUS Primitive Driver service.
     * @return true if a valid target was located and cached.
     */
    virtual bool discoverVehicle() = 0;
    /**
     * Begin the JAUS RequestControl exchange. Implementations should avoid
     * dispatching duplicate requests while one is pending.
     */
    virtual bool sendRequestControl() = 0;
    /** @return true if a JAUS request is waiting on a response. */
    virtual bool isRequestPending() const = 0;
    /** @return true once the remote server grants control authority. */
    virtual bool hasControl() const = 0;
    /**
     * Resume vehicle operation after a pause/standby state.
     * @return true when the command is successfully placed on the wire.
     */
    virtual bool sendRequestResume() = 0;
    /**
     * Query the remote system for its current management state.
     * @return true if the query message was queued for delivery.
     */
    virtual bool queryStatus() = 0;
    /** @return true when the last reported status equals READY. */
    virtual bool hasReadyState() const = 0;
    /** @return true if heartbeat monitoring indicates the peer is responsive. */
    virtual bool isHeartbeatAlive() const = 0;
    /** Stream calibrated joystick data to the remote primitive driver. */
    virtual void sendWrenchEffort(const frc_joystick_data_t& data) = 0;
    /** @return Human readable name of the selected remote component. */
    virtual std::string getComponentName() = 0;
};