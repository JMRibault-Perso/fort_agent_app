#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <fort_agent/coapHelpers.h>

// Facade for building CoAP messages to SRC Pro (and NSC unless noted).
// All CBOR payload encoding happens inside this class using tinycbor.
// Public API uses standard C++ types to keep call sites clean.

// Declare subscribe/unsubscribe wrappers for a base GET method
// Define subscribe/unsubscribe wrappers for a base GET method in a namespace
#define DECLARE_OBSERVE_METHODS(Name) \
    void subscribe##Name(uint16_t mid); \
    void unsubscribe##Name(uint16_t mid);

                   
namespace coapSRCPro {
    #define JB_MID 0x3000

    // Observe wrappers (use macro)
    DECLARE_OBSERVE_METHODS(CombinedJoystickKeypad)
    DECLARE_OBSERVE_METHODS(CpuTempC)
    DECLARE_OBSERVE_METHODS(DeviceTempC)
    DECLARE_OBSERVE_METHODS(GaugeTempC)
    DECLARE_OBSERVE_METHODS(GyroTempC)
    DECLARE_OBSERVE_METHODS(BatteryStatus)
    DECLARE_OBSERVE_METHODS(FirmwareVersion)
    DECLARE_OBSERVE_METHODS(SerialNumber)
    DECLARE_OBSERVE_METHODS(ModelNumber)
    DECLARE_OBSERVE_METHODS(DeviceMac) 

    // ---------- Strong URI type ----------
    struct Uri {
        std::vector<std::string> segments;
        Uri(std::initializer_list<std::string> s) : segments(s) {}
    }; 

    // ---------- Centralized URIs ----------
    namespace URIs {
        inline Uri SMCUSafety(int idx) { return {"sf", std::to_string(idx), "s"}; }
        inline Uri SMCUSafetyDiagnostics(int idx) { return {"sf", std::to_string(idx), "s", "safety_diagnostics"}; }
        inline Uri SMCUSystemDiagnostics(int idx) { return {"sf", std::to_string(idx), "s", "system_diagnostics"}; }
        inline Uri SMCUCombinedSafety{"sf","transmitter","combined"};

        // Device Info
        inline Uri RadioMode{"deviceInfo?radioMode"};
        inline Uri RadioPower{"deviceInfo?radioPowerDB"};
        inline Uri RadioChannel{"deviceInfo?radioChannel"};
        inline Uri RadioStatus{"deviceInfo?radioStatus"};
        inline Uri RadioUsed{"deviceInfo?radioUsed"};
        inline Uri FirmwareVersion{"deviceInfo?fwVersion"};
        inline Uri CpuTemp{"deviceInfo?cpuTempC"};
        inline Uri DeviceTemp{"deviceInfo?deviceTempC"};
        inline Uri GaugeTemp{"deviceInfo?gaugeTempC"};
        inline Uri GyroTemp{"deviceInfo?gyroTempC"};
        inline Uri BatteryStatus{"deviceInfo?batteryStatus"};
        inline Uri SystemStatus{"deviceInfo?sys1"};
        inline Uri LockdownStatus{"deviceInfo?lockdownStatus"};
        

        // Config
        inline Uri SerialNumber{"cfg","setup","serialNumber"};
        inline Uri ModelNumber{"cfg","setup","modelNumber"};
        inline Uri DeviceMac{"cfg","setup","deviceMac"};
        inline Uri DeviceUID{"cfg","setup","deviceUID"};
        inline Uri DeviceRev{"cfg","setup","deviceRev"};
        inline Uri SystemReset{"cfg","setup","systemReset"};
        inline Uri DisplayMode{"cfg","setup","userSettings?99"};
        inline Uri VibrateLeft{"cfg","setup","userSettings?10"};
        inline Uri VibrateRight{"cfg","setup","userSettings?11"};
        inline Uri VibrateBoth{"cfg","setup","userSettings?12"};
        inline Uri FirmareFileData(const std::string& filename) {
            return {"fs", "data?" + filename};
        };
        inline Uri FirmwareMetaData{"fs","metadata"};

        // State
        inline Uri JoystickCalibrated{"st","joystick","calibrated"};
        inline Uri Keypad{"st","keypad"};
        inline Uri CombinedJoystickKeypad{"st","joystick","combined"};
        inline Uri Mode{"st","mode"};
        inline Uri DisplayText{"st","display","text"};

        // Security
        inline Uri SecureElementID{"sec","dev","seuid"};
        inline Uri FsoId{"sec","dev","fso","id"};
        inline Uri FsoLength{"sec","dev","fso","length"};
        inline Uri FsoCrc{"sec","dev","fso","crc"};
        inline Uri FsoErase{"sec","dev","fso","erase"};
        inline Uri FsoData{"sec","dev","fso","data"};
        inline Uri Otp{"sec","lockdown","otp"};
        inline Uri LockdownProcessor{"sec","lockdown","processor"};
        inline Uri Scp03{"sec","lockdown","scp03"};
        inline Uri OtpWrite{"sec","lockdown","otpWrite"};
    } // namespace URIs

    // ---------- Public API declarations ----------
    // ---------- Safety Domain (sf) ----------
    // SMCU safety (application/octet-stream (42)), Observe on GET
    void getSMCUSafety(int smcuIndex, uint16_t mid,
                                       bool observe = true, uint16_t observeValue = 0);
    void postSMCUSafety(int smcuIndex, uint16_t mid,
                                        const std::vector<uint8_t>& raw);
    // Safety diagnostics (CBOR, Observe on GET)
    void getSMCUSafetyDiagnostics(int smcuIndex, uint16_t mid,
                                                  bool observe = false, uint16_t observeValue = 0);
    // System diagnostics (CBOR, Observe on GET)
    void getSMCUSystemDiagnostics(int smcuIndex, uint16_t mid,
                                                  bool observe = false, uint16_t observeValue = 0);
    // Combined safety (application/octet-stream (42), Observe on GET)
    void getSMCUCombinedSafety(uint16_t mid,
                                               bool observe = false, uint16_t observeValue = 0);

    // ---------- Device Info Domain (deviceInfo) ----------
    void getRadioMode(uint16_t mid);
    void getRadioPowerDb(uint16_t mid);
    void getRadioChannel(uint16_t mid);
    void getRadioStatus(uint16_t mid);
    void getRadioUsed(uint16_t mid);
    void getFirmwareVersion(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getCpuTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getDeviceTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getGaugeTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getGyroTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getBatteryStatus(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getSystemStatus(uint16_t mid);
    // Provisioning only
    void getLockdownStatus(uint16_t mid);

    // ---------- Config Domain (cfg/setup) ----------
    void getSerialNumber(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void postSerialNumber(uint16_t mid, const std::string& serial);
    void getModelNumber(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void postModelNumber(uint16_t mid, const std::string& model);
    void getDeviceMac(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    // Provisioning only
    void getDeviceUID(uint16_t mid);
    void getDeviceRev(uint16_t mid);
    void postSystemReset(uint16_t mid, char mode);
    // User settings
    void getDisplayMode(uint16_t mid);
    // payload should be a CBOR-encoded byte string of 0 or 1
    void postDisplayMode(uint16_t mid, uint8_t mode /*0 or 1*/);
    // Vibrate: left=10, right=11, both=12; CBOR-encoded byte string containing number 1
    void postVibrateLeft(uint16_t mid);
    void postVibrateRight(uint16_t mid);
    void postVibrateBoth(uint16_t mid);

    // ---------- File Endpoints (fs) ----------
    // Block-wise transfer; filename provided via query in URI segments
    void getFirmwareFileData(uint16_t mid, const std::string& filename);
    void postFirmwareFileData(uint16_t mid, const std::string& filename,
                                              const std::vector<uint8_t>& data);
    // Metadata: filename,length, crc32 via CBOR
    void getFirmwareFileMetadata(uint16_t mid);
    void postFirmwareFileMetadata(uint16_t mid,
                                                  const std::string& filename,
                                                  uint32_t length,
                                                  uint32_t crc32);

    // ---------- State Endpoints (st) ----------
    void getJoystickCalibrated(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getKeypad(uint16_t mid,
                                   bool observe = false, uint16_t observeValue = 0);
    void getCombinedJoystickKeypad(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    void getMode(uint16_t mid);
    // Display text: structured CBOR; support both simple 2-line write and segment write
    void postDisplayTextRawLines(uint16_t mid, const std::string& line0_18chars,
                const std::string& line1_18chars,
                bool upperHalf /*true=upper, false=lower*/);
    void postDisplayTextSegment(uint16_t mid, uint8_t line /*0..3*/,
                uint8_t segment /*0..2*/,
                const std::string& text6chars);

    // ---------- Security Endpoints (sec) ----------
    // Security related
    void getSecureElementID(uint16_t mid);

    // Device Provisioning - Factory Image Only (FSO)
    // FSO selection via POST string path component (id is part of stateful flow; expose setter)
    void postFsoId(uint16_t mid, const std::string& idString);
    void getFsoLength(uint16_t mid);
    void getFsoCrc(uint16_t mid);
    void getFsoErase(uint16_t mid);
    // FSO data transfer (DER) GET/POST
    void getFsoData(uint16_t mid);
    void postFsoData(uint16_t mid, const std::vector<uint8_t>& der);

    // Lockdown (OTP + processor + SCP03 + otpWrite)
    void getOtpKey(uint16_t mid);
    void postOtpCommit(uint16_t mid, const std::string& phrase,
                                       const std::string& seedHex);
    void getLockdownProcessorKey(uint16_t mid);
    void postLockdownProcessor(uint16_t mid, const std::string& phrase,
                                               const std::string& keyHex);
    void getScp03Rotate(uint16_t mid);
    // Dev test only
    void postOtpWriteDevTest(uint16_t mid, const std::string& asciiHex);
}; // namespace coapSRCPro