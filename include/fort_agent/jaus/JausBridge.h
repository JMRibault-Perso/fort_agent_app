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
        START = 900,
        SAFETY,
        DIAGNOSTICS,
        SAFETYCOMBINED,
        RADIOMODE,
        RADIOPOWER,
        RADIOCHANNEL,
        RADIOSTATUS,
        RADIOUSED,
        FIRMWAREVERSION,
        CPUTEMP,
        DEVICETEMP,
        GAUGETEMP,
        GYROTEMP,
        BATTERYSTATUS,
        SYSTEMSTATUS,
        LOCKDOWNSTATUS,
        SERIALNUMBER,
        MODELNUMBER,
        DEVICEMAC,
        DEVICEUID,
        DEVICEREV,
        SYSTEMRESET,
        DISPLAYMODE,
        VIBRATIONLEFT,
        VIBRATIONRIGHT,
        VIBRATIONBOTH,
        FIRMWAREFILEDATA,
        FIRMWAREFILEMETADATA,
        JOYSTICKCALIBRATED,
        KEYPAD,
        COMBINEDJOYSTICKKEYPAD,
        MODE,
        DISPLAYTEXT,
        SECUREELEMENTID,
        FSOID,
        FSODATA,
        FSOCRC,
        FSOERASE,
        FSOLENGTH,
        OPTKEY,
        OPTCOMMIT,
        LOCKDOWNPROCESSORKEY,
        SCP03ROTATE,
        OTPWRITEDEVTEST,
        END
    };

public:
    JausBridge(std::unique_ptr<JAUSClient> client);
    ~JausBridge() {
        stopServiceLoop(); // ensures cleanup
    }

    bool evaluateCoapJausMessage(JausBridge::JausPort jausPort, const uint8_t* payload, size_t payloadLen);
    void postInput(const frc_combined_data_t& input);
    void postJAUSResponse();
    void startServiceLoop();
    void stopServiceLoop();


private:
    struct BridgeMessage {
        enum class Kind { JoystickInput, JAUSResponse } kind;
        frc_combined_data_t joystickInput; // Serial port joystick input
    };

    std::string serialNumber;
    std::string modelNumber;
    BatteryStatus batteryStatus;

    std::unique_ptr<JAUSClient> jausClient;
    std::queue<BridgeMessage> inputQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    bool running = false;
    std::thread serviceThread;
    std::unique_ptr<VehicleStateMachine> stateMachine; 
    void serviceLoop();
};

