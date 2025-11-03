/**
 * @file FORTJoystickHelpers.h
 * @brief Strongly typed helpers for decoding Fort joystick and keypad payloads.
 */
#pragma once

#include <cstdint>
#include <cstddef> 
#include <string>

/** Number of data bits used in calibrated joystick samples. */
#define CALIBRATED_JS_DATA_BITS (12)
/** Remaining bit count dedicated to flags and reserved padding. */
#define CALIBRATED_JS_RESERVED_BITS (16 - 1 - CALIBRATED_JS_DATA_BITS) 

#pragma pack(push, 1)
/** Packed joystick sample with a validity flag supplied by the controller. */
typedef union {
    struct {
        signed int data : CALIBRATED_JS_DATA_BITS;
        bool dataOK : 1;
        unsigned int reserved : CALIBRATED_JS_RESERVED_BITS;
    };
    uint16_t asRaw;
} jsCal_t;
#pragma pack(pop)

#pragma pack(push, 1)
/** Aggregated calibrated joystick axes reported by the SRC Pro handheld. */
struct frc_joystick_data_t {
    jsCal_t leftXAxis; /**< Left X Axis Value */
    jsCal_t leftYAxis; /**< Left Y Axis Value */
    jsCal_t leftZAxis; /**< Left Z Axis Value */
    jsCal_t rightXAxis; /**< Right X Axis Value */
    jsCal_t rightYAxis; /**< Right Y Axis Value */
    jsCal_t rightZAxis; /**< Right Z Axis Value */
    uint16_t crc16;
    };
#pragma pack(pop)

#pragma pack(push, 1)
/* C/C++ style structural representation */
/** Keypad button payload coupled with a CRC-16 guard. */
struct frc_keypad_data_t {
    uint16_t buttonStatus; /**< Button status (As Bit representation defined above) */
    uint16_t crc16;
};
#pragma pack(pop)

#pragma pack(push, 1)
/** Composite report carrying keypad and joystick blocks back-to-back. */
struct frc_combined_data_t {
    frc_keypad_data_t keypad_data; 
    frc_joystick_data_t joystick_data;
};
#pragma pack(pop)

/** Human readable battery metrics parsed from SRC telemetry. */
struct BatteryStatus {
    int percent;
    double volts;
    double tempC;
    double amps;
};


/** Bit flag identifiers for each keypad button. */
enum class KeypadButton : uint16_t {
    Menu     = 1 << 0,
    Pause    = 1 << 1,
    Power    = 1 << 2,
    L_Down   = 1 << 3,
    L_Right  = 1 << 4,
    L_Up     = 1 << 5,
    L_Left   = 1 << 6,
    R_Down   = 1 << 7,
    R_Right  = 1 << 8,
    R_Up     = 1 << 9,
    R_Left   = 1 << 10
    };

/** Mode identifiers matching SRC Pro CoAP telemetry responses. */
enum SRCPMode : uint8_t {
    LOCAL_MODE = 0x04,          // SRCP is not connected to an output device.
    REMOTE_MODE = 0x06,         // SRCP is connected to an output device, but has not yet started sending joystick/buttons
    OPERATIONAL_MODE = 0x09,    // SRCP is connected to an output device and is sending joystick/buttons
    MENU_MODE = 0x0A,           // SRCP is currently in a menu, and will send 0’d joystick/buttons
    PAUSE_MODE = 0x0B,          // SRCP is currently paused, and will send 0’d joystick/buttons
    DISPLAY_TEXT_MODE = 0x0E    // SRCP is in User Display Text screen
    };

/** Quick bitmask helper for checking keypad button state. */
inline bool isButtonPressed(uint16_t status, KeypadButton button) {
    return (status & static_cast<uint16_t>(button)) != 0;
}

/** Dump joystick and keypad state to stdout for debugging. */
void printJoystickStatus(const frc_combined_data_t& js);
/** Compute CRC-16 used by joystick and keypad payloads. */
uint16_t compute_crc16(const uint8_t* data, size_t size);
/** Decode a combined report and update cached state structures. */
bool decode_combined_payload(const uint8_t* data, size_t size);  
/** Extract battery telemetry values from a CBOR payload. */
BatteryStatus decode_battery_payload(const uint8_t* payload, size_t size);

/** Render text on the SRC Pro user display. */
void displayTextOnJoystick(const std::string& text, const std::string& subtext);
/** Trigger joystick vibration motors. */
void vibrateJoystick(bool leftMotor, bool rightMotor);
