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

    namespace detail {
        template<Coap::Method Method, Coap::Format Format, JausBridge::JausPort Port>
        void sendRequest(uint16_t mid, const Uri& uri,
                        const std::vector<uint8_t>& payload,
                        bool observe, uint16_t observeValue) {
            auto coapMsg = build(Method, mid, uri, payload, Format, observe, observeValue);
            UartCoapBridgeSingleton::instance().sendSRCRequest(coapMsg, static_cast<uint16_t>(Port));
        }

        template<Coap::Method Method, Coap::Format Format, JausBridge::JausPort Port>
        void sendRequest(uint16_t mid, const Uri& uri,
                        const std::vector<uint8_t>& payload) {
            sendRequest<Method, Format, Port>(mid, uri, payload, false, 0);
        }

        template<Coap::Method Method, Coap::Format Format, JausBridge::JausPort Port>
        void sendRequest(uint16_t mid, const Uri& uri,
                        bool observe = false, uint16_t observeValue = 0) {
            static const std::vector<uint8_t> emptyPayload;
            sendRequest<Method, Format, Port>(mid, uri, emptyPayload, observe, observeValue);
        }

        inline std::vector<uint8_t> asciiPayload(const std::string& text) {
            return std::vector<uint8_t>(text.begin(), text.end());
        }
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
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::SAFETY>(
            mid, URIs::SMCUSafety(smcuIndex), observe, observeValue);
    }

    void postSMCUSafety(int smcuIndex, uint16_t mid,
                                                    const std::vector<uint8_t>& raw) {
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::SAFETY>(
            mid, URIs::SMCUSafety(smcuIndex), raw);
    }

    void getSMCUSafetyDiagnostics(int smcuIndex, uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DIAGNOSTICS>(
            mid, URIs::SMCUSafetyDiagnostics(smcuIndex), observe, observeValue);
    }

    void getSMCUSystemDiagnostics(int smcuIndex, uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DIAGNOSTICS>(
            mid, URIs::SMCUSystemDiagnostics(smcuIndex), observe, observeValue);
    }

    void getSMCUCombinedSafety(uint16_t mid,
                                                        bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::SAFETYCOMBINED>(
            mid, URIs::SMCUCombinedSafety, observe, observeValue);
    }

    // -------------------- Device Info Domain --------------------
    void getRadioMode(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::RADIOMODE>(
            mid, URIs::RadioMode);
    }

    void getRadioPowerDb(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::RADIOPOWER>(
            mid, URIs::RadioPower);
    }

    void getRadioChannel(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::RADIOCHANNEL>(
            mid, URIs::RadioChannel);
    }

    void getRadioStatus(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::RADIOSTATUS>(
            mid, URIs::RadioChannel);
    }

    void getRadioUsed(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::RADIOUSED>(
            mid, URIs::RadioUsed);
    }

    void getFirmwareVersion(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FIRMWAREVERSION>(
            mid, URIs::FirmwareVersion, observe, observeValue);
    }

    void getCpuTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::CPUTEMP>(
            mid, URIs::CpuTemp, observe, observeValue);
    }

    void getDeviceTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::DEVICETEMP>(
            mid, URIs::DeviceTemp, observe, observeValue);
    }

    void getGaugeTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::GAUGETEMP>(
            mid, URIs::GaugeTemp, observe, observeValue);
    }

    void getGyroTempC(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::GYROTEMP>(
            mid, URIs::GyroTemp, observe, observeValue);
    }

    void getBatteryStatus(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        // Spec says "text/plain (0)" but content described as CBOR object; we respect the table's format field.
        // If the implementation expects CBOR, adjust Coap::Format::APPLICATION_CBOR here.
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::BATTERYSTATUS>(
            mid, URIs::BatteryStatus, observe, observeValue);
    }

    void getSystemStatus(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::SYSTEMSTATUS>(
            mid, URIs::SystemStatus);
    }

    void getLockdownStatus(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::LOCKDOWNSTATUS>(
            mid, URIs::LockdownStatus);
    }

    // -------------------- Config Domain --------------------
    void getSerialNumber(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::SERIALNUMBER>(
            mid, URIs::SerialNumber, observe, observeValue);
    }

    void postSerialNumber(uint16_t mid, const std::string& serial) {
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::SERIALNUMBER>(
            mid, URIs::SerialNumber, detail::asciiPayload(serial));
    }

    void getModelNumber(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::MODELNUMBER>(
            mid, URIs::ModelNumber, observe, observeValue);
    }

    void postModelNumber(uint16_t mid, const std::string& model) {
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::MODELNUMBER>(
            mid, URIs::ModelNumber, detail::asciiPayload(model));
    }

    void getDeviceMac(uint16_t mid,
                                       bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::DEVICEMAC>(
            mid, URIs::DeviceMac, observe, observeValue);
    }

    void getDeviceUID(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::DEVICEUID>(
            mid, URIs::DeviceUID);
    }

    void getDeviceRev(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::DEVICEREV>(
            mid, URIs::DeviceRev);
    }

    void postSystemReset(uint16_t mid, char mode) {
        // ASCII 'n' or 'b'
        std::vector<uint8_t> payload{static_cast<uint8_t>(mode)};
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::SYSTEMRESET>(
            mid, URIs::SystemReset, payload);
    }

    void getDisplayMode(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DISPLAYMODE>(
            mid, URIs::DisplayMode);
    }

    void postDisplayMode(uint16_t mid, uint8_t mode) {
        // payload is CBOR-encoded byte string containing number 0 or 1 (single byte)
        vector<uint8_t> raw{mode};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DISPLAYMODE>(
            mid, URIs::DisplayMode, payload);
    }

    void postVibrateLeft(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::VIBRATIONLEFT>(
            mid, URIs::VibrateLeft, payload);
    }

    void postVibrateRight(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::VIBRATIONRIGHT>(
            mid, URIs::VibrateRight, payload);
    }

    void postVibrateBoth(uint16_t mid) {
        vector<uint8_t> raw{1};
        auto payload = CBORHelpers::cborEncodeByteString(raw);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::VIBRATIONBOTH>(
            mid, URIs::VibrateBoth, payload);
    }

    // -------------------- File Endpoints --------------------
    void getFirmwareFileData(uint16_t mid, const std::string& filename) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::FIRMWAREFILEDATA>(
            mid, URIs::FirmareFileData(filename));
    }

    void postFirmwareFileData(uint16_t mid, const std::string& filename,
                                                        const std::vector<uint8_t>& data) {
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::FIRMWAREFILEDATA>(
            mid, URIs::FirmareFileData(filename), data);
    }

    void getFirmwareFileMetadata(uint16_t mid) {
        // Example: fs/metadata?computed (optional query); we provide base endpoint and allow caller to add query if needed
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::FIRMWAREFILEMETADATA>(
            mid, URIs::FirmwareMetaData);
    }

    void postFirmwareFileMetadata(uint16_t mid,
                                                            const std::string& filename,
                                                            uint32_t length,
                                                            uint32_t crc32) {
        auto payload = CBORHelpers::cborEncodeFileMetadata(filename, length, crc32);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::FIRMWAREFILEMETADATA>(
            mid, URIs::FirmwareMetaData, payload);
    }

    // -------------------- State Endpoints --------------------
    void getJoystickCalibrated(uint16_t mid,
                                                        bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::JOYSTICKCALIBRATED>(
            mid, URIs::JoystickCalibrated, observe, observeValue);
    }

    void getKeypad(uint16_t mid,
                                            bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::KEYPAD>(
            mid, URIs::Keypad, observe, observeValue);
    }

    void getCombinedJoystickKeypad(uint16_t mid,
                                                            bool observe, uint16_t observeValue) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::APPLICATION_OCTET_STREAM, JausBridge::JausPort::COMBINEDJOYSTICKKEYPAD>(
            mid, URIs::CombinedJoystickKeypad, observe, observeValue);
    }

    void getMode(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::MODE>(
            mid, URIs::Mode);
    }

    void postDisplayTextRawLines(uint16_t mid,
                                                            const std::string& line0_18chars,
                                                            const std::string& line1_18chars,
                                                            bool upperHalf) {
        auto payload = CBORHelpers::cborEncodeTwoLines18(line0_18chars, line1_18chars, upperHalf);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DISPLAYTEXT>(
            mid, URIs::DisplayText, payload);
    }

    void postDisplayTextSegment(uint16_t mid,
                                                            uint8_t line, uint8_t segment,
                                                            const std::string& text6chars) {
        auto payload = CBORHelpers::cborEncodeDisplaySegment(line, segment, text6chars);
        detail::sendRequest<Coap::Method::POST, Coap::Format::APPLICATION_CBOR, JausBridge::JausPort::DISPLAYTEXT>(
            mid, URIs::DisplayText, payload);
    }

    // -------------------- Security Endpoints --------------------
    void getSecureElementID(uint16_t mid) {
        // ASCII encoded hex
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::SECUREELEMENTID>(
            mid, URIs::SecureElementID);
    }

    // FSO selection/id
    void postFsoId(uint16_t mid, const std::string& idString) {
        // POST String (plain text)
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSOID>(
            mid, URIs::FsoId, detail::asciiPayload(idString));
    }

    void getFsoLength(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSOLENGTH>(
            mid, URIs::FsoLength);
    }

    void getFsoCrc(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSOCRC>(
            mid, URIs::FsoCrc);
    }

    void getFsoErase(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSOERASE>(
            mid, URIs::FsoErase);
    }

    void getFsoData(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSODATA>(
            mid, URIs::FsoData);
    }

    void postFsoData(uint16_t mid, const std::vector<uint8_t>& der) {
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::FSODATA>(
            mid, URIs::FsoData, der);
    }

    // Lockdown OTP flow
    void getOtpKey(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::OPTKEY>(
            mid, URIs::Otp);
    }

    void postOtpCommit(uint16_t mid, const std::string& phrase,
                                                const std::string& seedHex) {
        // ASCII encoded hex: phrase and seed packed as "<phrase> <seed>"
        string payloadStr = phrase + " " + seedHex;
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::OPTCOMMIT>(
            mid, URIs::Otp, detail::asciiPayload(payloadStr));
    }

    void getLockdownProcessorKey(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::LOCKDOWNPROCESSORKEY>(
            mid, URIs::LockdownProcessor);
    }

    void postLockdownProcessor(uint16_t mid, const std::string& phrase,
                                                        const std::string& keyHex) {
        // ASCII encoded hex "<phrase> <key>"
        string payloadStr = phrase + " " + keyHex;
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::LOCKDOWNPROCESSORKEY>(
            mid, URIs::LockdownProcessor, detail::asciiPayload(payloadStr));
    }

    void getScp03Rotate(uint16_t mid) {
        detail::sendRequest<Coap::Method::GET, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::SCP03ROTATE>(
            mid, URIs::Scp03);
    }

    void postOtpWriteDevTest(uint16_t mid, const std::string& asciiHex) {
        // Development Test Only, ASCII encoded hex via POST
        detail::sendRequest<Coap::Method::POST, Coap::Format::TEXT_PLAIN, JausBridge::JausPort::OTPWRITEDEVTEST>(
            mid, URIs::OtpWrite, detail::asciiPayload(asciiHex));
    }
}