#pragma once

#include <cstring>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <fort_agent/jaus/JausClient.h>
#include <fort_agent/jaus/vehicleStateMachine.h>
#include <fort_agent/uart/FORTJoystick/FORTJoystickHelpers.h>

class JausBridge
{
public:
    enum class JausPort : uint16_t {
        KEYPAD = 900,
        CALIBRATED_JS = 901,
        COMBINED_JS = 1000,
        SRCP_MODE = 1001,
        DISPLAY_TEXT = 1002
    };

public:
    JausBridge(std::unique_ptr<JAUSClient> client);
    ~JausBridge() {
        stopServiceLoop(); // ensures cleanup
    }

    bool EvaluateCoapJausMessage(JausBridge::JausPort jausPort, const uint8_t* payload, size_t payloadLen);
    void postInput(const frc_combined_data_t& input);
    void startServiceLoop();
    void stopServiceLoop();

private:
    std::unique_ptr<JAUSClient> jausClient;

    std::queue<frc_combined_data_t> inputQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    bool running = false;
    std::thread serviceThread;
    std::unique_ptr<VehicleStateMachine> stateMachine; 
    void serviceLoop();
};

