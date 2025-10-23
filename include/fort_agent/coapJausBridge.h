#pragma once
#include <cstring>

#include <fort_agent/dbgTrace.h>

/* C/C++ style structural representation */
#define CALIBRATED_JS_DATA_BITS (12)
#define CALIBRATED_JS_RESERVED_BITS (16 - 1 - CALIBRATED_JS_DATA_BITS) 

class JausBridge
{
private:
    enum class SRCPMode : uint8_t {
        LOCAL_MODE = 0x04,          // SRCP is not connected to an output device.
        REMOTE_MODE = 0x06,         // SRCP is connected to an output device, but has not yet started sending joystick/buttons
        OPERATIONAL_MODE = 0x09,    // SRCP is connected to an output device and is sending joystick/buttons
        MENU_MODE = 0x0A,           // SRCP is currently in a menu, and will send 0’d joystick/buttons
        PAUSE_MODE = 0x0B,          // SRCP is currently paused, and will send 0’d joystick/buttons
        DISPLAY_TEXT_MODE = 0x0E    // SRCP is in User Display Text screen
    };

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

    void printJoystickStatus(const frc_combined_data_t& js);

public:
    static uint16_t compute_crc16(const uint8_t* data, size_t size);
    bool EvaluateCoapJausMessage(uint16_t port, const uint8_t* payload, size_t payloadLen);
    
private:

    void decode_keypad_payload(const uint8_t* data, size_t size);
    void decode_calibrated_joystick_payload(const uint8_t* data, size_t size);
    void decode_combined_payload(const uint8_t* data, size_t size); 
    void decode_display_text(const uint8_t* data, size_t size);
    void decode_srcp_mode(const uint8_t* data, size_t size);

    void send_jaus_mode(const std::string& mode);
    void send_jaus_display_text(const std::string& line1, const std::string& line2);
    void send_jaus_keypad_state(uint16_t buttonStatus);
    void send_jaus_calibrated_joystick(const struct frc_joystick_data_t& js);
    void send_jaus_combined_joystick(const struct frc_combined_data_t& js);   

};