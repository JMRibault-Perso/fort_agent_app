#ifndef FORT_AGENT_CPP_COAPHELPERS_H
#define FORT_AGENT_CPP_COAPHELPERS_H

#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

#include <fort_agent/dbgTrace.h>

class CoapException : public std::runtime_error {
public:
    explicit CoapException(const std::string &what) :
        std::runtime_error(what) {}
};

namespace Coap {
    enum class Type : uint8_t {
        CON = 0,
        NON,
        ACK,
        RST
    };

    enum class Method : uint8_t {
        GET = 0x01,
        POST = 0x02
    };

    enum class Format : uint16_t {
        TEXT_PLAIN = 0,
        APPLICATION_LINK_FORMAT = 40,
        APPLICATION_XML = 41,
        APPLICATION_OCTET_STREAM = 42,
        APPLICATION_EXI = 47,
        APPLICATION_JSON = 50,
        APPLICATION_CBOR = 60,
        NONE = 0xFFFF
    };

    struct CoapReply {
        uint16_t mid;
        std::vector<uint8_t> token;
        std::vector<uint8_t> payload;
    };


    // Building
    std::vector<uint8_t> buildMessage(Type type, Method method, uint16_t mid,
                                    const std::vector<std::string>& uriSegments,
                                    const std::vector<uint8_t>& payload,
                                    const std::vector<uint8_t>& token,
                                    Format contentFormat = Format::NONE,
                                    bool includeObserve = false,
                                    uint16_t observeValue = 0);


    // Parsing
    Type getCoapType(const uint8_t *coapBuffer);

    uint8_t getTokenLength(const uint8_t *coapBuffer);

    uint8_t getCode(const uint8_t *coapBuffer, uint8_t *codeClass = nullptr,
                    uint8_t *codeDetail = nullptr);

    bool isEmpty(const uint8_t *coapBuffer);

    bool isRequest(const uint8_t *coapBuffer);

    bool isResponse(const uint8_t *coapBuffer);

    uint16_t getMid(const uint8_t *coapBuffer);

    bool looksLikeCoap(const uint8_t *buffer, size_t len);

    // Modification / Generation
    void setTokenLength(uint8_t *coapBuffer, uint8_t tokenLen);

    // Parse reply
    CoapReply parseObserveReply(const uint8_t* buffer, size_t len);

    std::array<uint8_t, 4> createResetMsg(uint16_t mid);

    // Definitions
    constexpr auto midTimeoutTime = std::chrono::seconds(
        250); /* CoAP RFC 7252 4.8.2 max exchange lifetime == 247s */
    constexpr size_t COAP_MIN_LEN = 4;
    constexpr size_t COAP_TOKEN_MAX_LEN = 8;
    constexpr size_t COAP_TOKEN_START_INDEX = 4;


}

#endif //FORT_AGENT_CPP_COAPHELPERS_H
