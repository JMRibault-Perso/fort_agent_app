/**
 * @file coapSRCPro.h
 * @brief High level CoAP request builders targeting the SRC Pro joystick.
 */
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <fort_agent/coapHelpers.h>

// Facade for building CoAP messages to SRC Pro (and NSC unless noted).
// All CBOR payload encoding happens inside this class using tinycbor.
// Public API uses standard C++ types to keep call sites clean.

/** Declare subscribe/unsubscribe helpers for observable resources. */
#define DECLARE_OBSERVE_METHODS(Name) \
    void subscribe##Name(uint16_t mid); \
    void unsubscribe##Name(uint16_t mid);

                   
/** Message builder helpers intended for communicating with SRC Pro. */
namespace coapSRCPro {
    /** Default message ID base used by joystick bridge. */
    #define JS_MID 0x3000

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
    /** Strongly typed URI segments for composing CoAP paths. */
    struct Uri {
        std::vector<std::string> segments;
        Uri(std::initializer_list<std::string> s) : segments(s) {}
    }; 

    // ---------- Centralized URIs ----------
    namespace URIs {
        /**
         * @brief Helpers returning canonical URIs for SRC Pro resources.
         *
         * Functions that require runtime formatting return a Uri; constants are
         * exposed as inline variables for direct reuse.
         */
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
    /** Request raw SMCU safety payload, optionally enabling observation. */
    void getSMCUSafety(int smcuIndex, uint16_t mid,
                                       bool observe = true, uint16_t observeValue = 0);
    /** Push raw safety block bytes back to the SMCU slot. */
    void postSMCUSafety(int smcuIndex, uint16_t mid,
                                        const std::vector<uint8_t>& raw);
    // Safety diagnostics (CBOR, Observe on GET)
    /** Fetch CBOR diagnostics for the given SMCU index. */
    void getSMCUSafetyDiagnostics(int smcuIndex, uint16_t mid,
                                                  bool observe = false, uint16_t observeValue = 0);
    // System diagnostics (CBOR, Observe on GET)
    /** Fetch system diagnostics data for the given SMCU index. */
    void getSMCUSystemDiagnostics(int smcuIndex, uint16_t mid,
                                                  bool observe = false, uint16_t observeValue = 0);
    // Combined safety (application/octet-stream (42), Observe on GET)
    /** Stream the shared combined transmitter safety block. */
    void getSMCUCombinedSafety(uint16_t mid,
                                               bool observe = false, uint16_t observeValue = 0);

    // ---------- Device Info Domain (deviceInfo) ----------
    /** Query current radio operating mode. */
    void getRadioMode(uint16_t mid);
    /** Query configured radio output power in dB. */
    void getRadioPowerDb(uint16_t mid);
    /** Query selected radio channel. */
    void getRadioChannel(uint16_t mid);
    /** Query radio health/status flags. */
    void getRadioStatus(uint16_t mid);
    /** Query whether the radio link is currently in use. */
    void getRadioUsed(uint16_t mid);
    /** Fetch firmware version string, optionally enabling observe. */
    void getFirmwareVersion(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch CPU temperature in Celsius. */
    void getCpuTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch device chassis temperature in Celsius. */
    void getDeviceTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch gauge temperature in Celsius. */
    void getGaugeTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch gyro temperature in Celsius. */
    void getGyroTempC(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch aggregated battery status structure. */
    void getBatteryStatus(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Query global system status bitfield. */
    void getSystemStatus(uint16_t mid);
    // Provisioning only
    /** Provisioning helper reporting lockdown state. */
    void getLockdownStatus(uint16_t mid);

    // ---------- Config Domain (cfg/setup) ----------
    /** Fetch the configured serial number string. */
    void getSerialNumber(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Update the device serial number field. */
    void postSerialNumber(uint16_t mid, const std::string& serial);
    /** Fetch the configured model number string. */
    void getModelNumber(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Update the model number field. */
    void postModelNumber(uint16_t mid, const std::string& model);
    /** Fetch device MAC address string. */
    void getDeviceMac(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    // Provisioning only
    /** Retrieve unique device identifier (factory only). */
    void getDeviceUID(uint16_t mid);
    /** Retrieve device hardware revision string. */
    void getDeviceRev(uint16_t mid);
    /** Issue system reset, providing reset mode character. */
    void postSystemReset(uint16_t mid, char mode);
    // User settings
    /** Read current user display mode. */
    void getDisplayMode(uint16_t mid);
    // payload should be a CBOR-encoded byte string of 0 or 1
    /** Set user display mode (0 = normal, 1 = alternate). */
    void postDisplayMode(uint16_t mid, uint8_t mode /*0 or 1*/);
    // Vibrate: left=10, right=11, both=12; CBOR-encoded byte string containing number 1
    /** Vibrate only the left motor. */
    void postVibrateLeft(uint16_t mid);
    /** Vibrate only the right motor. */
    void postVibrateRight(uint16_t mid);
    /** Vibrate both motors simultaneously. */
    void postVibrateBoth(uint16_t mid);

    // ---------- File Endpoints (fs) ----------
    // Block-wise transfer; filename provided via query in URI segments
    /** Start a block-wise GET for firmware file contents. */
    void getFirmwareFileData(uint16_t mid, const std::string& filename);
    /** Upload firmware file chunk data. */
    void postFirmwareFileData(uint16_t mid, const std::string& filename,
                                              const std::vector<uint8_t>& data);
    // Metadata: filename,length, crc32 via CBOR
    /** Retrieve metadata describing the current firmware file. */
    void getFirmwareFileMetadata(uint16_t mid);
    /** Upload firmware metadata (filename, length, CRC32). */
    void postFirmwareFileMetadata(uint16_t mid,
                                                  const std::string& filename,
                                                  uint32_t length,
                                                  uint32_t crc32);

    // ---------- State Endpoints (st) ----------
    /** Fetch joystick calibration status bit. */
    void getJoystickCalibrated(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch keypad button payload. */
    void getKeypad(uint16_t mid,
                                   bool observe = false, uint16_t observeValue = 0);
    /** Fetch combined joystick and keypad snapshot. */
    void getCombinedJoystickKeypad(uint16_t mid, bool observe = false, uint16_t observeValue = 0);
    /** Fetch current SRC operating mode enumeration. */
    void getMode(uint16_t mid);
    // Display text: structured CBOR; support both simple 2-line write and segment write
    /**
     * @brief Update two 18-character lines on the user display.
     * @param line0_18chars First line of display text.
     * @param line1_18chars Second line of display text.
     * @param upperHalf true targets upper display region, false targets lower.
     */
    void postDisplayTextRawLines(uint16_t mid, const std::string& line0_18chars,
                const std::string& line1_18chars,
                bool upperHalf /*true=upper, false=lower*/);
    /** Post a 6-character segment within a display quadrant. */
    void postDisplayTextSegment(uint16_t mid, uint8_t line /*0..3*/,
                uint8_t segment /*0..2*/,
                const std::string& text6chars);

    // ---------- Security Endpoints (sec) ----------
    // Security related
    /** Query secure element unique identifier. */
    void getSecureElementID(uint16_t mid);

    // Device Provisioning - Factory Image Only (FSO)
    // FSO selection via POST string path component (id is part of stateful flow; expose setter)
    /** Select factory secure object ID for subsequent requests. */
    void postFsoId(uint16_t mid, const std::string& idString);
    /** Retrieve FSO length in bytes. */
    void getFsoLength(uint16_t mid);
    /** Retrieve FSO CRC32 value. */
    void getFsoCrc(uint16_t mid);
    /** Trigger FSO erase operation. */
    void getFsoErase(uint16_t mid);
    // FSO data transfer (DER) GET/POST
    /** Download FSO DER data. */
    void getFsoData(uint16_t mid);
    /** Upload FSO DER payload. */
    void postFsoData(uint16_t mid, const std::vector<uint8_t>& der);

    // Lockdown (OTP + processor + SCP03 + otpWrite)
    /** Retrieve OTP key material (factory only). */
    void getOtpKey(uint16_t mid);
    /** Finalize OTP provisioning using phrase and seed. */
    void postOtpCommit(uint16_t mid, const std::string& phrase,
                                       const std::string& seedHex);
    /** Retrieve lockdown processor key. */
    void getLockdownProcessorKey(uint16_t mid);
    /** Program lockdown processor secret. */
    void postLockdownProcessor(uint16_t mid, const std::string& phrase,
                                               const std::string& keyHex);
    /** Request SCP03 key rotation. */
    void getScp03Rotate(uint16_t mid);
    // Dev test only
    /** Developer-only helper to write raw OTP data. */
    void postOtpWriteDevTest(uint16_t mid, const std::string& asciiHex);
}; // namespace coapSRCPro