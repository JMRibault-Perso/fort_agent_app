#pragma once

#include <cstdint>
#include <cstddef> 
#include <string>

/* C/C++ style structural representation */
#define CALIBRATED_JS_DATA_BITS (12)
#define CALIBRATED_JS_RESERVED_BITS (16 - 1 - CALIBRATED_JS_DATA_BITS) 

#pragma pack(push, 1)
/// Calibrated joystick data, includes a field for if the data is valid
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
struct frc_joystick_data_t {
    jsCal_t leftXAxis; /* Left X Axis Value */
    jsCal_t leftYAxis; /* Left Y Axis Value */
    jsCal_t leftZAxis; /* Left Z Axis Value */
    jsCal_t rightXAxis; /* Right X Axis Value */
    jsCal_t rightYAxis; /* Right Y Axis Value */
    jsCal_t rightZAxis; /* Right Z Axis Value */
    uint16_t crc16;
    };
#pragma pack(pop)

#pragma pack(push, 1)
/* C/C++ style structural representation */
struct frc_keypad_data_t {
    uint16_t buttonStatus; /* Button status (As Bit representation defined above) */
    uint16_t crc16;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct frc_combined_data_t {
    frc_keypad_data_t keypad_data; 
    frc_joystick_data_t joystick_data;
};
#pragma pack(pop)

struct BatteryStatus {
    int percent;
    double volts;
    double tempC;
    double amps;
};


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

enum SRCPMode : uint8_t {
    LOCAL_MODE = 0x04,          // SRCP is not connected to an output device.
    REMOTE_MODE = 0x06,         // SRCP is connected to an output device, but has not yet started sending joystick/buttons
    OPERATIONAL_MODE = 0x09,    // SRCP is connected to an output device and is sending joystick/buttons
    MENU_MODE = 0x0A,           // SRCP is currently in a menu, and will send 0’d joystick/buttons
    PAUSE_MODE = 0x0B,          // SRCP is currently paused, and will send 0’d joystick/buttons
    DISPLAY_TEXT_MODE = 0x0E    // SRCP is in User Display Text screen
    };

inline bool isButtonPressed(uint16_t status, KeypadButton button) {
    return (status & static_cast<uint16_t>(button)) != 0;
}

void printJoystickStatus(const frc_combined_data_t& js);
uint16_t compute_crc16(const uint8_t* data, size_t size);
bool decode_combined_payload(const uint8_t* data, size_t size);  
BatteryStatus decode_battery_payload(const uint8_t* payload, size_t size);

void displayTextOnJoystick(const std::string& text, const std::string& subtext);
void vibrateJoystick(bool leftMotor, bool rightMotor);
