#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>
#include <fort_agent/fort_agent.h>

#include <iostream>
#include <iomanip>
#include <cstdint>


void printJoystickStatus(const frc_combined_data_t& js) {
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

uint16_t compute_crc16(const uint8_t* data, size_t size) {
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

bool decode_combined_payload(const uint8_t* data, size_t size) {
    if (size < sizeof(frc_combined_data_t)) {
        std::cerr << "Combined payload too small\n";
        return false;
    }

    frc_combined_data_t payload;
    std::memcpy(&payload, data, sizeof(payload));

    // Compute and verify CRCs
    uint16_t expected_joystick_crc = compute_crc16((const uint8_t*)&payload.joystick_data, sizeof(frc_joystick_data_t) - 2);
    if (expected_joystick_crc != payload.joystick_data.crc16) {
        std::cerr << "Combined Joystick CRC mismatch\n";
        return false;
    }
    uint16_t expected_keypad_crc = compute_crc16((const uint8_t*)&payload.keypad_data.buttonStatus, sizeof(uint16_t));
    if (expected_keypad_crc != payload.keypad_data.crc16) {
        std::cerr << "Combined Keypad CRC mismatch\n";
        return false;
    }

    // printJoystickStatus(payload);
    return true;
}
