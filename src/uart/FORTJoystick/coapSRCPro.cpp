#include <cbor.h>
#include <algorithm>
#include <fort_agent/uart/FORTJoystick/coapSRCPro.h>
#include <fort_agent/uartCoapBridgeSingleton.h>
#include <fort_agent/jaus/JausBridgeSingleton.h>

using namespace std;



    
namespace coapSRCPro {

#define DEFINE_OBSERVE_METHODS(Name) \
    void subscribe##Name(uint16_t mid) { \
        get##Name(mid, true, 0); \
    } \
    void unsubscribe##Name(uint16_t mid) { \
        get##Name(mid, true, 1); \
    }

        // Observe wrappers (use macro)
    DEFINE_OBSERVE_METHODS(CombinedJoystickKeypad)
    DEFINE_OBSERVE_METHODS(CpuTempC)
    DEFINE_OBSERVE_METHODS(DeviceTempC)
    DEFINE_OBSERVE_METHODS(GaugeTempC)
    DEFINE_OBSERVE_METHODS(GyroTempC)
    DEFINE_OBSERVE_METHODS(BatteryStatus)
    DEFINE_OBSERVE_METHODS(FirmwareVersion)
    DEFINE_OBSERVE_METHODS(SerialNumber)
    DEFINE_OBSERVE_METHODS(ModelNumber)
    DEFINE_OBSERVE_METHODS(DeviceMac) 

    inline std::vector<uint8_t> build(Coap::Method method,
                                uint16_t mid,
                                const Uri& uri,
                                const std::vector<uint8_t>& payload = {},
                                Coap::Format fmt = Coap::Format::TEXT_PLAIN,
                                bool observe = false,
                                uint16_t observeValue = 0) {
        return Coap::buildMessage(Coap::Type::CON, method, mid,
                                uri.segments, payload, {}, fmt, observe, observeValue);
        }

    namespace CBORHelpers {
        // -------------------- CBOR helpers --------------------
        std::vector<uint8_t> cborEncodeUint(uint64_t value) {
            vector<uint8_t> buf(16);
            CborEncoder enc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encode_uint(&enc, value);
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        std::vector<uint8_t> cborEncodeInt(int64_t value) {
            vector<uint8_t> buf(16);
            CborEncoder enc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encode_int(&enc, value);
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        std::vector<uint8_t> cborEncodeText(const std::string& s) {
            vector<uint8_t> buf(s.size() + 16);
            CborEncoder enc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encode_text_stringz(&enc, s.c_str());
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        std::vector<uint8_t> cborEncodeByteString(const std::vector<uint8_t>& bytes) {
            vector<uint8_t> buf(bytes.size() + 16);
            CborEncoder enc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encode_byte_string(&enc, bytes.data(), bytes.size());
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        // CBOR for firmware metadata: a simple CBOR map { "filename": <text>, "length": <uint>, "crc32": <uint> }
        std::vector<uint8_t> cborEncodeFileMetadata(const std::string& filename,
                                                                uint32_t length,
                                                                uint32_t crc32) {
            vector<uint8_t> buf(filename.size() + 32);
            CborEncoder enc, mapEnc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encoder_create_map(&enc, &mapEnc, 3);
            cbor_encode_text_stringz(&mapEnc, "filename");
            cbor_encode_text_stringz(&mapEnc, filename.c_str());
            cbor_encode_text_stringz(&mapEnc, "length");
            cbor_encode_uint(&mapEnc, length);
            cbor_encode_text_stringz(&mapEnc, "crc32");
            cbor_encode_uint(&mapEnc, crc32);
            cbor_encoder_close_container(&enc, &mapEnc);
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        // CBOR display segment: { "display_text": [ line, segment, "payload" ] }
        // The spec shows a structure with field name display_text: byte + string; encode as array for compactness.
        std::vector<uint8_t> cborEncodeDisplaySegment(uint8_t line, uint8_t segment,
                                                                const std::string& text6) {
            string t = text6.substr(0, min<size_t>(6, text6.size()));
            vector<uint8_t> buf(t.size() + 32);
            CborEncoder enc, mapEnc, arrEnc;
            cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
            cbor_encoder_create_map(&enc, &mapEnc, 1);
            cbor_encode_text_stringz(&mapEnc, "display_text");
            cbor_encoder_create_array(&mapEnc, &arrEnc, 3);
            cbor_encode_uint(&arrEnc, line);
            cbor_encode_uint(&arrEnc, segment);
            cbor_encode_text_stringz(&arrEnc, t.c_str());
            cbor_encoder_close_container(&mapEnc, &arrEnc);
            cbor_encoder_close_container(&enc, &mapEnc);
            size_t n = cbor_encoder_get_buffer_size(&enc, buf.data());
            buf.resize(n);
            return buf;
        }

        // CBOR raw two lines (18 chars each) + trailing byte to pick upper/lower half.
        // Encode as byte string: [ line0(18) | line1(18) | half(1) ]
        std::vector<uint8_t> cborEncodeTwoLines18(const std::string& l0,
                                                            const std::string& l1,
                                                            bool upperHalf) {
            auto s0 = l0.substr(0, min<size_t>(18, l0.size()));
            auto s1 = l1.substr(0, min<size_t>(18, l1.size()));
            vector<uint8_t> raw;
            raw.reserve(18 + 18 + 1);
            raw.insert(raw.end(), s0.begin(), s0.end());
            raw.insert(raw.end(), s1.begin(), s1.end());
            raw.push_back(upperHalf ? 1 : 0);
            return cborEncodeByteString(raw);
        }
    }

    // -------------------- Safety Domain --------------------
    void getSMCUSafety(int smcuIndex, uint16_t mid, bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SMCUSafety(smcuIndex),
                    {}, Coap::Format::APPLICATION_OCTET_STREAM, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SAFETY);
    }

    void postSMCUSafety(int smcuIndex, uint16_t mid,
                                                    const std::vector<uint8_t>& raw) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::SMCUSafety(smcuIndex),
                    raw, Coap::Format::APPLICATION_OCTET_STREAM, false, 0);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SAFETY);
    }

    void getSMCUSafetyDiagnostics(int smcuIndex, uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SMCUSafetyDiagnostics(smcuIndex),
                    {}, Coap::Format::APPLICATION_CBOR, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DIAGNOSTICS);
    }

    void getSMCUSystemDiagnostics(int smcuIndex, uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SMCUSystemDiagnostics(smcuIndex),
                    {}, Coap::Format::APPLICATION_CBOR, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DIAGNOSTICS);
    }

    void getSMCUCombinedSafety(uint16_t mid,
                                                        bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SMCUCombinedSafety,
                    {}, Coap::Format::APPLICATION_OCTET_STREAM, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SAFETYCOMBINED);
    }

    // -------------------- Device Info Domain --------------------
    void getRadioMode(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::RadioMode, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::RADIOMODE);
    }

    void getRadioPowerDb(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::RadioPower, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::RADIOPOWER);
    }

    void getRadioChannel(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::RadioChannel, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::RADIOCHANNEL);
    }

    void getRadioStatus(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::RadioChannel, {}, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::RADIOSTATUS);
    }

    void getRadioUsed(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::RadioUsed, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::RADIOUSED);
    }

    void getFirmwareVersion(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FirmwareVersion, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FIRMWAREVERSION);
    }

    void getCpuTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::CpuTemp, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::CPUTEMP);
    }

    void getDeviceTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::DeviceTemp, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DEVICETEMP);
    }

    void getGaugeTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::GaugeTemp, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::GAUGETEMP);
    }

    void getGyroTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::GyroTemp, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::GYROTEMP);
    }

    void getBatteryStatus(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        // Spec says "text/plain (0)" but content described as CBOR object; we respect the table's format field.
        // If the implementation expects CBOR, adjust Coap::Format::APPLICATION_CBOR here.
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::BatteryStatus, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::BATTERYSTATUS);
    }

    void getSystemStatus(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SystemStatus, {}, Coap::Format::APPLICATION_OCTET_STREAM);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SYSTEMSTATUS);
    }

    void getLockdownStatus(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::LockdownStatus, {}, Coap::Format::APPLICATION_OCTET_STREAM);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::LOCKDOWNSTATUS);
    }

    // -------------------- Config Domain --------------------
    void getSerialNumber(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SerialNumber, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SERIALNUMBER);
    }

    void postSerialNumber(uint16_t mid, const std::string& serial) {
        vector<uint8_t> payload(serial.begin(), serial.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::SerialNumber, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SERIALNUMBER);
    }

    void getModelNumber(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::ModelNumber, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::MODELNUMBER);
    }

    void postModelNumber(uint16_t mid, const std::string& model) {
        vector<uint8_t> payload(model.begin(), model.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::ModelNumber, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::MODELNUMBER);
    }

    void getDeviceMac(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::DeviceMac, {}, Coap::Format::TEXT_PLAIN, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DEVICEMAC);
    }

    void getDeviceUID(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::DeviceUID, {}, Coap::Format::APPLICATION_OCTET_STREAM);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DEVICEUID);
    }

    void getDeviceRev(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::DeviceRev, {}, Coap::Format::APPLICATION_OCTET_STREAM);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DEVICEREV);
    }

    void postSystemReset(uint16_t mid, char mode) {
        // ASCII 'n' or 'b'
        vector<uint8_t> payload{static_cast<uint8_t>(mode)};
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::SystemReset, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SYSTEMRESET);
    }

    void getDisplayMode(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::DisplayMode, {}, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DISPLAYMODE);
    }

    void postDisplayMode(uint16_t mid, uint8_t mode) {
        // payload is CBOR-encoded byte string containing number 0 or 1 (single byte)
        vector<uint8_t> raw{mode};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::DisplayMode, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DISPLAYMODE);
    }

    void postVibrateLeft(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::VibrateLeft, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::VIBRATIONLEFT);
    }

    void postVibrateRight(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::VibrateRight, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::VIBRATIONRIGHT);
    }

    void postVibrateBoth(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::VibrateBoth, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::VIBRATIONBOTH);
    }

    // -------------------- File Endpoints --------------------
    void getFirmwareFileData(uint16_t mid, const std::string& filename) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FirmareFileData(filename), {}, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FIRMWAREFILEDATA);
    }

    void postFirmwareFileData(uint16_t mid, const std::string& filename,
                                                        const std::vector<uint8_t>& data) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::FirmareFileData(filename), data, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FIRMWAREFILEDATA);
    }

    void getFirmwareFileMetadata(uint16_t mid) {
        // Example: fs/metadata?computed (optional query); we provide base endpoint and allow caller to add query if needed
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FirmwareMetaData, {}, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FIRMWAREFILEMETADATA);
    }

    void postFirmwareFileMetadata(uint16_t mid,
                                                            const std::string& filename,
                                                            uint32_t length,
                                                            uint32_t crc32) {
        auto payload = CBORHelpers::cborEncodeFileMetadata(filename, length, crc32);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::FirmwareMetaData, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FIRMWAREFILEMETADATA);
    }

    // -------------------- State Endpoints --------------------
    void getJoystickCalibrated(uint16_t mid,
                                                        bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::JoystickCalibrated, {}, Coap::Format::APPLICATION_OCTET_STREAM, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::JOYSTICKCALIBRATED);
    }

    void getKeypad(uint16_t mid,
                                            bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::Keypad, {}, Coap::Format::APPLICATION_OCTET_STREAM, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::KEYPAD);
    }

    void getCombinedJoystickKeypad(uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::CombinedJoystickKeypad, {}, Coap::Format::APPLICATION_OCTET_STREAM, observe, observeValue);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::COMBINEDJOYSTICKKEYPAD);
    }

    void getMode(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::Mode, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::MODE);
    }

    void postDisplayTextRawLines(uint16_t mid,
                                                            const std::string& line0_18chars,
                                                            const std::string& line1_18chars,
                                                            bool upperHalf) {
        auto payload = CBORHelpers::cborEncodeTwoLines18(line0_18chars, line1_18chars, upperHalf);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::DisplayText, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DISPLAYTEXT);
    }

    void postDisplayTextSegment(uint16_t mid,
                                                            uint8_t line, uint8_t segment,
                                                            const std::string& text6chars) {
        auto payload = CBORHelpers::cborEncodeDisplaySegment(line, segment, text6chars);
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::DisplayText, payload, Coap::Format::APPLICATION_CBOR);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::DISPLAYTEXT);
    }

    // -------------------- Security Endpoints --------------------
    void getSecureElementID(uint16_t mid) {
        // ASCII encoded hex
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::SecureElementID, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SECUREELEMENTID);
    }

    // FSO selection/id
    void postFsoId(uint16_t mid, const std::string& idString) {
        // POST String (plain text)
        vector<uint8_t> payload(idString.begin(), idString.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::FsoId, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSOID);
    }

    void getFsoLength(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FsoLength, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSOLENGTH);
    }

    void getFsoCrc(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FsoCrc, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSOCRC);
    }

    void getFsoErase(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FsoErase, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSOERASE);
    }

    void getFsoData(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::FsoData, {}, Coap::Format::TEXT_PLAIN /*DER - opaque*/);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSODATA);
    }

    void postFsoData(uint16_t mid, const std::vector<uint8_t>& der) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::FsoData, der, Coap::Format::TEXT_PLAIN /*DER - opaque*/);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::FSODATA);
    }

    // Lockdown OTP flow
    void getOtpKey(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::Otp, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::OPTKEY);
    }

    void postOtpCommit(uint16_t mid, const std::string& phrase,
                                                const std::string& seedHex) {
        // ASCII encoded hex: phrase and seed packed as "<phrase> <seed>"
        string payloadStr = phrase + " " + seedHex;
        vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::Otp, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::OPTCOMMIT);
    }

    void getLockdownProcessorKey(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::LockdownProcessor, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::LOCKDOWNPROCESSORKEY);
    }

    void postLockdownProcessor(uint16_t mid, const std::string& phrase,
                                                        const std::string& keyHex) {
        // ASCII encoded hex "<phrase> <key>"
        string payloadStr = phrase + " " + keyHex;
        vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::LockdownProcessor, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::LOCKDOWNPROCESSORKEY);
    }

    void getScp03Rotate(uint16_t mid) {
        std::vector<uint8_t> coapMsg = build(Coap::Method::GET, mid,
                    URIs::Scp03, {}, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::SCP03ROTATE);
    }

    void postOtpWriteDevTest(uint16_t mid, const std::string& asciiHex) {
        // Development Test Only, ASCII encoded hex via POST
        vector<uint8_t> payload(asciiHex.begin(), asciiHex.end());
        std::vector<uint8_t> coapMsg = build(Coap::Method::POST, mid,
                    URIs::OtpWrite, payload, Coap::Format::TEXT_PLAIN);
        UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, (uint16_t) JausBridge::JausPort::OTPWRITEDEVTEST);
    }
}