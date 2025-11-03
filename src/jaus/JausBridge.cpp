// #include <signal.h>
#include <iostream>
#include <iomanip>
#include <cstdint>

#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>
#include <fort_agent/jaus/JausBridge.h>
#include <fort_agent/jaus/JausBridgeSingleton.h>
#include <fort_agent/jaus/states/InitializeState.h>
#include <fort_agent/uartCoapBridgeSingleton.h>
#include <fort_agent/dbgTrace.h>

#include <fort_agent/uart/FORTJoystick/coapSRCPro.h>

#define JS_MID 0x3000
using namespace coapSRCPro;

JausBridge::JausBridge(std::unique_ptr<JAUSClient> client) : 
    jausClient(std::move(client))  {
    stateMachine = nullptr;    
    // Constructor implementation
}

void JausBridge::startServiceLoop() {
    // Let's setup the Joystick uart modes here
    
    // Verbose stuff
    coapSRCPro::getSerialNumber(JS_MID);
    coapSRCPro::getModelNumber(JS_MID);
    //coapSRCPro::getFirmwareVersion(JS_MID);

    // send Observe request for joystick/combined resource
    coapSRCPro::unsubscribeCombinedJoystickKeypad(JS_MID);
    coapSRCPro::subscribeCombinedJoystickKeypad(JS_MID);


    //coapSRCPro::getBatteryStatus(JS_MID); // observe battery status

    //coapSRCPro::unsubscribeBatteryStatus(JS_MID);
    //coapSRCPro::subscribeBatteryStatus(JS_MID); 

    // send Observe request for SRCP Mode resource
    //sendObserveSRCPModeRequest(1);
    //sendObserveSRCPModeRequest(0);
    //coapSRCPro::getSystemStatus(JS_MID);

    coapSRCPro::postVibrateBoth(JS_MID); // post vibration

    //coapSRCPro::getCpuTempC(JS_MID); // observe CPU temp

    // Initialize state machine with InitializeState
    stateMachine = std::make_unique<VehicleStateMachine>(
    std::make_unique<InitializeState>(*jausClient));

    jausClient->initializeJAUS(); // Initialize JAUS client before starting loop
    running = true;
    serviceThread = std::thread(&JausBridge::serviceLoop, this);
}

void JausBridge::stopServiceLoop() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }
    queueCV.notify_all();
    if (serviceThread.joinable()) {
        serviceThread.join();
    }
}

void JausBridge::serviceLoop() {
    while (running) {
        BridgeMessage msg;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [&] { return !inputQueue.empty() || !running; });

            if (!running) break;
            msg = inputQueue.front();
            inputQueue.pop();
        }

        switch (msg.kind) {
            case BridgeMessage::Kind::JoysticInput:
                stateMachine->handleInput(msg.joystickInput);
                break;
            case BridgeMessage::Kind::JAUSResponse:
                stateMachine->handleResponse();
                break;
        }
    }
}


bool JausBridge::evaluateCoapJausMessage(JausBridge::JausPort jausPort, const uint8_t* payload, size_t payloadLen) {
    bool ret = true; // Assume we handled the message

    switch(jausPort) {
        case JausPort::SAFETY: // Safety
            break;
        case JausPort::DIAGNOSTICS: // Diagnostics
            break;
        case JausPort::SAFETYCOMBINED: // Safety Combined
            break;
        case JausPort::RADIOMODE: // Radio Mode
            break;
        case JausPort::RADIOPOWER: // Radio Power
            break;
        case JausPort::RADIOCHANNEL: // Radio Channel
            break;
        case JausPort::RADIOSTATUS: // Radio Status
            break;
        case JausPort::RADIOUSED: // Radio Used
            break;
        case JausPort::FIRMWAREVERSION: // Firmware Version
            if (payloadLen != 0) {
                std::string firmware(reinterpret_cast<const char*>(payload), payloadLen);
                std::cout << "JAUS: Received Firmware Version: " << firmware << std::endl;
            }
            break;
        case JausPort::CPUTEMP: // CPU Temp
            if (payloadLen != 0) {
                std::cout << "JAUS: CPU Temperature: " << payload << std::endl;
            }
            break;
        case JausPort::DEVICETEMP: // Device Temp
            break;
        case JausPort::GAUGETEMP: // Gauge Temp
            break;
        case JausPort::GYROTEMP: // Gyro Temp
            break;
        case JausPort::BATTERYSTATUS: // Battery Status
            if (payloadLen != 0) {
                batteryStatus = decode_battery_payload(payload, payloadLen);
                spdlog::debug("JAUS: Battery Status - {}% | {:.2f}V | {:.2f}C | {:.2f}A",
                    batteryStatus.percent, batteryStatus.volts, batteryStatus.tempC, batteryStatus.amps);
                }
            break;
        case JausPort::SYSTEMSTATUS: // System Status
            if (payloadLen != 0) {
                std::string systemStatus = std::string(reinterpret_cast<const char*>(payload), payloadLen);
                std::cout << "JAUS: System Status: " << systemStatus << std::endl;
            }
            break;
        case JausPort::LOCKDOWNSTATUS: // Lockdown Status
            break;
        case JausPort::SERIALNUMBER: // Serial Number
            if (payloadLen != 0) {
                std::string serial(reinterpret_cast<const char*>(payload), payloadLen);
                serialNumber = serial;
                spdlog::debug("JAUS: Received Serial Number: {}", serial);
                std::cout << "JAUS: Received Serial Number: " << serial << std::endl;
            }
            break;
        case JausPort::MODELNUMBER: // Model Number
            if (payloadLen != 0) {
                std::string model(reinterpret_cast<const char*>(payload), payloadLen);
                modelNumber = model;
                spdlog::debug("JAUS: Received Model Number: {}", model);
                std::cout << "JAUS: Received Model Number: " << model << std::endl;
            }
            break;
        case JausPort::DEVICEMAC: // Device MAC
            break;
        case JausPort::DEVICEUID: // Device UID
            break;
        case JausPort::DEVICEREV: // Device Rev
            break;
        case JausPort::SYSTEMRESET: // System Reset
            break;
        case JausPort::DISPLAYMODE: // Display Mode
                if (payloadLen != 0) {
                    spdlog::debug("JAUS: Received Display Mode payload of length {}", payloadLen);
                }
            break;
        case JausPort::VIBRATIONLEFT: // Vibration Left
            break;
        case JausPort::VIBRATIONRIGHT: // Vibration Right
            break;
        case JausPort::VIBRATIONBOTH: // Vibration Both
            break;
        case JausPort::FIRMWAREFILEDATA: // Firmware File Data
            break;
        case JausPort::FIRMWAREFILEMETADATA: // Firmware File Metadata
            break;
        case JausPort::KEYPAD : // Keypad
            break;
        case JausPort::JOYSTICKCALIBRATED : // Calibrated Joystick
            break;
        case JausPort::COMBINEDJOYSTICKKEYPAD: // Combined Joystick
            if (decode_combined_payload(payload, payloadLen) ){
                spdlog::debug("JAUS: Decoded combined joystick payload successfully");
                postInput(*reinterpret_cast<const frc_combined_data_t*>(payload));
            } else {
                spdlog::warn("JAUS: Failed to decode combined joystick payload");                       
            }
            break;
        case JausPort::FSODATA: // FSO Data
            break;
        case JausPort::OPTKEY: // OTP Key
            break;
        case JausPort::OPTCOMMIT: // OTP Commit
            break;
        case JausPort::LOCKDOWNPROCESSORKEY: // Lockdown Processor Key
            break;
        case JausPort::SCP03ROTATE: // SCP03 Rotate
            break;
        case JausPort::OTPWRITEDEVTEST: // OTP Write Dev Test
            break;
        case JausPort::DISPLAYTEXT : // Display Text
            break;
        default:
            spdlog::warn("JAUS: Unknown port {}, cannot handle message", static_cast<uint16_t>(jausPort));
            ret = false; // Did not handle the message
            break;
    }
    return ret;
    }

void JausBridge::postJAUSResponse() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        BridgeMessage msg;
        msg.kind = BridgeMessage::Kind::JAUSResponse;
        inputQueue.push(msg);
    }
    queueCV.notify_one();
}


void JausBridge::postInput(const frc_combined_data_t& input) {
    static frc_combined_data_t lastJs = {};

    if (std::memcmp(&input, &lastJs, sizeof(frc_combined_data_t)) == 0) {
        return; // No change, no need to post constantly
    }
    lastJs = input;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        BridgeMessage msg;
        msg.joystickInput = input;
        msg.kind = BridgeMessage::Kind::JoysticInput;
        inputQueue.push(msg);
    }
    queueCV.notify_one();
}

/*
void JausBridge::sendDisplayLineText(int lineIndex, const std::string& text) {
    FXN_TRACE;

    if (lineIndex < 0 || lineIndex > 3) {
        spdlog::debug("sendDisplayLineText: Invalid line index: {}", lineIndex);
        return;
    }

    constexpr size_t segmentCount = 3;
    constexpr size_t segmentSize = 6;

    for (uint8_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
        size_t offset = segmentIndex * segmentSize;
        if (offset >= text.size()) {
            break; // No more text to send
        }

        std::string segmentText = text.substr(offset, segmentSize);

        // Prepare fixed-length payload
        std::array<uint8_t, segmentSize> padded{};
        std::memcpy(padded.data(), segmentText.c_str(), std::min(segmentText.size(), padded.size()));

        // Build binary payload: [lineIndex][segmentIndex][6-byte payload]
        std::vector<uint8_t> binaryPayload;
        binaryPayload.push_back(static_cast<uint8_t>(lineIndex));
        binaryPayload.push_back(segmentIndex);
        binaryPayload.insert(binaryPayload.end(), padded.begin(), padded.end());

        // Build CoAP POST message
        std::vector<uint8_t> coapMsg = Coap::buildMessage(
            Coap::Type::CON,
            Coap::Method::POST,
            0x4321 + segmentIndex, // Unique MID per segment
            {"st", "display", "text"},
            binaryPayload,
            {}, // No token
            Coap::Format::APPLICATION_OCTET_STREAM,
            false, // No observe
            0
        );

        sendRequest(coapMsg, 1002);
    }
}
    */

