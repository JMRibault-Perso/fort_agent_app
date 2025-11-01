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


JausBridge::JausBridge(std::unique_ptr<JAUSClient> client) : 
    jausClient(std::move(client))  {
    stateMachine = nullptr;    
    // Constructor implementation
}

void JausBridge::startServiceLoop() {

    // Initialize state machine with InitializeState
    stateMachine = std::make_unique<VehicleStateMachine>(
    std::make_unique<InitializeState>(*jausClient, UartCoapBridgeSingleton::instance()));

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
        frc_combined_data_t input;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [&] { return !inputQueue.empty() || !running; });

            if (!running) break;
            input = inputQueue.front();
            inputQueue.pop();
        }

        stateMachine->handleInput(input); // now takes frc_combined_data_t
    }
}


bool JausBridge::EvaluateCoapJausMessage(JausBridge::JausPort jausPort, const uint8_t* payload, size_t payloadLen) {
    bool ret = true; // Assume we handled the message

    switch(jausPort) {
        case JausPort::KEYPAD : // Keypad
            break;
        case JausPort::CALIBRATED_JS : // Calibrated Joystick
            break;
        case JausPort::COMBINED_JS: // Combined Joystick
            if (decode_combined_payload(payload, payloadLen) ){
                spdlog::debug("JAUS: Decoded combined joystick payload successfully");
                postInput(*reinterpret_cast<const frc_combined_data_t*>(payload));
            } else {
                spdlog::warn("JAUS: Failed to decode combined joystick payload");                       
            }
            break;
        case JausPort::SRCP_MODE: // SRCP Mode
            break;
        case JausPort::DISPLAY_TEXT: // Display Text
            break;
        default:
            spdlog::warn("JAUS: Unknown port {}, cannot handle message", jausPort);
            break;
    }
    
    return ret;
    }



void JausBridge::postInput(const frc_combined_data_t& input) {
    static frc_combined_data_t lastJs = {};

    if (std::memcmp(&input, &lastJs, sizeof(frc_combined_data_t)) == 0) {
        return; // No change, no need to post constantly
    }
    lastJs = input;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        inputQueue.push(input);
    }
    queueCV.notify_one();
}

