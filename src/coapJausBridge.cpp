#include <fort_agent/fort_agent.h>
#include <fort_agent/coapJausBridge.h>
#include <iostream>
#include <iomanip>
#include <cstdint>



bool JausBridge::EvaluateCoapJausMessage(uint16_t port, const uint8_t* payload, size_t payloadLen) {
    bool ret = true; // Assume we handled the message


    if (payloadLen == 0) {
        spdlog::warn("JAUS: No payload found in CoAP message");
        //return ret;
    }

    switch(port) {
        case 900: // Keypad
            decode_keypad_payload(payload, payloadLen);
            ret = false; // Message was handled
            break;
        case 901: // Calibrated Joystick
            decode_calibrated_joystick_payload(payload, payloadLen);
            ret = false; // Message was handled
            break;
        case 1000: // Combined Joystick
            decode_combined_payload(payload, payloadLen);
            ret = false; // Message was handled
            break;
        case 1001: // SRCP Mode
            decode_srcp_mode(payload, payloadLen);
            break;
        case 1002: // Display Text
            //decode_display_text(payload, payloadLen);
            ret = false; // Message was handled
            break;
        default:
            spdlog::warn("JAUS: Unknown port {}, cannot handle message", port);
            break;
    }
    
    return ret;
    }

void JausBridge::send_jaus_mode(const std::string& mode) {
    /*
    openjaus::core::Component comp;
    comp.initialize();

    openjaus::messages::ReportMode msg;
    msg.setModeString(mode);  // You may want to map to enum

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));
    */
}

void JausBridge::send_jaus_display_text(const std::string& line1, const std::string& line2) {
    /*
    openjaus::core::Component comp;
    comp.initialize();

    openjaus::messages::ReportDisplayText msg;
    msg.setLine(1, line1);
    msg.setLine(2, line2);

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));
    */
}

uint8_t reverseBits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}


uint16_t JausBridge::compute_crc16(const uint8_t* data, size_t size) {
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
  
    return crc;
}

void JausBridge::decode_srcp_mode(const uint8_t* data, size_t size) {
    if (size < 1) {
        std::cerr << "SRCP mode payload too small\n";
        return;
    }

    JausBridge::SRCPMode modeVal = static_cast<JausBridge::SRCPMode>(data[0]);
    std::string modeStr;
    switch(modeVal) {
        case SRCPMode::LOCAL_MODE:
            modeStr = "LOCAL_MODE";
            break;
        case SRCPMode::REMOTE_MODE:
            modeStr = "REMOTE_MODE";
            break;
        case SRCPMode::OPERATIONAL_MODE:
            modeStr = "OPERATIONAL_MODE";
            break;
        case SRCPMode::MENU_MODE:
            modeStr = "MENU_MODE";
            break;
        case SRCPMode::PAUSE_MODE:
            modeStr = "PAUSE_MODE";
            break;
        case SRCPMode::DISPLAY_TEXT_MODE:
            modeStr = "DISPLAY_TEXT_MODE";
            break;
        default:
            modeStr = "UNKNOWN_MODE";
            break;
    }

    std::cout << "JAUS: Received SRCP mode: " << modeStr << std::endl;
    std::string mode(reinterpret_cast<const char*>(data), size);
    send_jaus_mode(mode);
}


void JausBridge::decode_keypad_payload(const uint8_t* data, size_t size) {
    if (size < sizeof(frc_keypad_data_t)) {
        std::cerr << "Keypad payload too small\n";
        return;
    }

    uint16_t expected_crc = compute_crc16(data, size - 2);
    uint16_t received_crc = *(uint16_t*)(data + size - 2);
    if (expected_crc != received_crc) {
        std::cerr << "Keypad CRC mismatch\n";
        return;
    }

    frc_keypad_data_t payload;
    std::memcpy(&payload, data, sizeof(payload));
    send_jaus_keypad_state(payload.buttonStatus);
}

void JausBridge::decode_calibrated_joystick_payload(const uint8_t* data, size_t size) {
    if (size < sizeof(frc_joystick_data_t)) {
        std::cerr << "Joystick payload too small\n";
        return;
    }

    uint16_t expected_crc = compute_crc16(data, size - 2);
    uint16_t received_crc = *(uint16_t*)(data + size - 2);
    if (expected_crc != received_crc) {
        std::cerr << "Joystick CRC mismatch\n";
        return;
    }

    frc_joystick_data_t payload;
    std::memcpy(&payload, data, sizeof(payload));
    send_jaus_calibrated_joystick(payload);
}

void JausBridge::send_jaus_keypad_state(uint16_t buttonStatus) {
    /*
    openjaus::core::Component comp;
    comp.initialize();

    openjaus::messages::ReportKeypadState msg;
    msg.setButtonMap(buttonStatus);

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));
    */
}

void JausBridge::send_jaus_calibrated_joystick(const frc_joystick_data_t& js) {
    /*
    openjaus::core::Component comp;
    comp.initialize();

    openjaus::messages::ReportCalibratedJoystick msg;
    msg.setLeftJoystick(js.leftXAxis, js.leftYAxis, js.leftZAxis);
    msg.setRightJoystick(js.rightXAxis, js.rightYAxis, js.rightZAxis);

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));
    */
}

void JausBridge::printJoystickStatus(const frc_combined_data_t& js) {
    static frc_combined_data_t lastJs = {};
    if (spdlog::get_level() != spdlog::level::info) {
        return;
    }

    static auto lastPrint = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (now - lastPrint < std::chrono::milliseconds(100)) return;
    lastPrint = now;

    if (std::memcmp(&js, &lastJs, sizeof(frc_combined_data_t)) == 0) {
        return; // No change
    }
    lastJs = js;
    
    printToConsole.flushString("\033[2J\033[3J\033[H\n");
    std::ostringstream frame;

    auto decode = [](jsCal_t val) -> std::string {
        int16_t value = static_cast<int16_t>(val.data);
        bool valid = val.dataOK;
        return (valid ? "[OK] " : "[--] ") + std::to_string(value);
    };

    auto decodeButtons = [&](uint16_t status) {
        const char* names[] = {
            "Menu", "Pause", "Power",
            "L-Down", "L-Right", "L-Up", "L-Left",
            "R-Down", "R-Right", "R-Up", "R-Left"
        };

        frame << "🕹️ Button Status\n";
        frame << "----------------\n";

        for (int i = 0; i < 11; ++i) {
            bool pressed = status & (1 << i);
            frame << std::setfill(' ') << std::left << std::setw(10) << names[i]
                << ": " << (pressed ? "Pressed" : "Released") << "                          \n";
        }
    };

    frame << "🎮 Joystick Status\n";
    frame << "------------------\n";
    frame << "Left  X: " << decode(js.joystick_data.leftXAxis) << "                          \n";
    frame << "Left  Y: " << decode(js.joystick_data.leftYAxis) << "                          \n";
    frame << "Left  Z: " << decode(js.joystick_data.leftZAxis) << "                          \n";
    frame << "Right X: " << decode(js.joystick_data.rightXAxis) << "                          \n";
    frame << "Right Y: " << decode(js.joystick_data.rightYAxis) << "                          \n";
    frame << "Right Z: " << decode(js.joystick_data.rightZAxis) << "                          \n";

    frame << "Joystick data CRC16 : 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << js.joystick_data.crc16 << "  cal: 0x"
        << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << compute_crc16((const uint8_t*)&js.joystick_data, sizeof(frc_joystick_data_t) - 2)
        << "\n";
    frame << "------------------\n";

    decodeButtons(js.keypad_data.buttonStatus);

    frame << "Keypad CRC16 : 0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << js.keypad_data.crc16 << " cal: 0x"
        << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << compute_crc16((const uint8_t*)&js.keypad_data.buttonStatus, sizeof(uint16_t))
        << "                          \n";
    frame << "------------------\n";

    // Final flush
    printToConsole.flushString(frame.str());
}


void JausBridge::decode_combined_payload(const uint8_t* data, size_t size) {
    if (size != sizeof(frc_combined_data_t)) {
    if (size < sizeof(frc_combined_data_t)) {
        std::cerr << "Combined payload too small\n";
        return;
        }
        else {
            std::cerr << "Combined payload too large\n";
        }
    }

    frc_combined_data_t payload;
    std::memcpy(&payload, data, sizeof(payload));

    // Compute and verify CRCs
    uint16_t expected_joystick_crc = compute_crc16((const uint8_t*)&payload.joystick_data, sizeof(frc_joystick_data_t) - 2);
    if (expected_joystick_crc != payload.joystick_data.crc16) {
        std::cerr << "Combined Joystick CRC mismatch\n";
        return;
    }
    uint16_t expected_keypad_crc = compute_crc16((const uint8_t*)&payload.keypad_data.buttonStatus, sizeof(uint16_t));
    if (expected_keypad_crc != payload.keypad_data.crc16) {
        std::cerr << "Combined Keypad CRC mismatch\n";
        return;
    }

    printJoystickStatus(payload);
    send_jaus_combined_joystick(payload);
}

void JausBridge::send_jaus_combined_joystick(const struct frc_combined_data_t& js) {
    /*
    openjaus::core::Component comp;
    comp.initialize();

    openjaus::messages::ReportCalibratedJoystick msg;
    msg.setLeftJoystick(js.leftXAxis, js.leftYAxis, js.leftZAxis);
    msg.setRightJoystick(js.rightXAxis, js.rightYAxis, js.rightZAxis);

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));

    openjaus::messages::ReportKeypadState msg;
    msg.setButtonMap(buttonStatus);

    comp.sendMessage(msg, openjaus::core::Address(1001, 1, 1));
    */
}
