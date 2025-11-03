#include <fort_agent/coapHelpers.h>
#include <spdlog/fmt/ostr.h>
#include <boost/crc.hpp>

using fmt::format;

Coap::Type Coap::getCoapType(const uint8_t *coapBuffer) {
    return static_cast<Coap::Type>((coapBuffer[0] >> 4) & 0x3);
}

uint8_t Coap::getTokenLength(const uint8_t *coapBuffer) {
    return coapBuffer[0] & 0x0f;
}

void Coap::setTokenLength(uint8_t *coapBuffer, uint8_t tokenLen) {
    if (tokenLen > 8) {
        throw CoapException(format(
            "Attempted to set invalid token length ({})", tokenLen));
    }
    coapBuffer[0] &= 0xf0;
    coapBuffer[0] |= (tokenLen & 0x0f);
}

uint8_t Coap::getCode(const uint8_t *coapBuffer, uint8_t *codeClass,
                      uint8_t *codeDetail) {
    if (codeClass != nullptr) {
        *codeClass = (coapBuffer[1] >> 5) & 0x7;
    }
    if (codeDetail != nullptr) {
        *codeDetail = coapBuffer[1] & 0x1f;
    }

    return coapBuffer[1];
}

/* Code value ranges: Class can be 0-7, Detail can be 0-31
 *   0.00       Empty
 *   0.01-0.31  Request
 *   1.00-1.31  reserved
 *   2.00-5.31  Response
 *   6.00-7.31  reserved
 */
bool Coap::isEmpty(const uint8_t *coapBuffer) {
    return getCode(coapBuffer) == 0;
}

bool Coap::isRequest(const uint8_t *coapBuffer) {
    uint8_t codeClass = 0;
    uint8_t codeDetail = 0;
    getCode(coapBuffer, &codeClass, &codeDetail);

    return (codeClass == 0 && codeDetail >= 1);
}

bool Coap::isResponse(const uint8_t *coapBuffer) {
    uint8_t codeClass = 0;
    getCode(coapBuffer, &codeClass);

    return (codeClass >= 2 && codeClass <= 5);
}

uint16_t Coap::getMid(const uint8_t *coapBuffer) {
    return ((static_cast<uint16_t>(coapBuffer[2]) << 8) & 0xff00) |
           (static_cast<uint16_t>(coapBuffer[3]) & 0x00ff);
}

bool Coap::looksLikeCoap(const uint8_t *buffer, size_t len) {
    const size_t tokenLen = getTokenLength(buffer);
    return (len >= COAP_MIN_LEN + tokenLen &&  // minimum CoAP message size
            (buffer[0] & 0xC0) == 0x40 && // CoAP version field is 01
            tokenLen <= 8); // maximum allowed token length
}


//------------------------------------------------------------------------------


std::array<uint8_t, 4> Coap::createResetMsg(uint16_t mid) {
    /*     0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |Ver| T |  TKL  |      Code     |          Message ID           |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  01   3     0          0.00            network byte order
     */
    return std::array<uint8_t, 4>{0x70,
                                  0x00,
                                  static_cast<uint8_t>((mid >> 8) & 0xff),
                                  static_cast<uint8_t>(mid & 0xff)};
}


static void encodeOption(std::vector<uint8_t>& out, uint16_t number, const std::string& value, uint16_t& lastNumber) {
    uint16_t delta = number - lastNumber;
    lastNumber = number;

    uint8_t deltaNibble = (delta < 13) ? delta : 13;
    uint8_t lenNibble = (value.size() < 13) ? value.size() : 13;

    out.push_back((deltaNibble << 4) | lenNibble);
    out.insert(out.end(), value.begin(), value.end());
}

static void encodeUintOption(std::vector<uint8_t>& out, uint16_t number, uint16_t value, uint16_t& lastNumber) {
    uint16_t delta = number - lastNumber;
    lastNumber = number;

    uint8_t len = (value <= 0xFF) ? 1 : 2;
    uint8_t deltaNibble = (delta < 13) ? delta : 13;
    uint8_t lenNibble = len;

    out.push_back((deltaNibble << 4) | lenNibble);
    if (len == 1) {
        out.push_back(static_cast<uint8_t>(value));
    } else {
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }
}

static uint16_t computeCRC16(const std::vector<uint8_t>& data) {
    boost::crc_16_type crc;
    crc.process_bytes(data.data(), data.size());
    return crc.checksum();
}

std::vector<uint8_t> Coap::buildMessage(Type type, Method method, uint16_t mid,
                                        const std::vector<std::string>& uriSegments,
                                        const std::vector<uint8_t>& payload,
                                        const std::vector<uint8_t>& token,
                                        Format contentFormat,
                                        bool includeObserve,
                                        uint16_t observeValue) {
    std::vector<uint8_t> msg;

    // Header: Ver(2) | Type(2) | TKL(4)
    uint8_t header = (1 << 6) | (static_cast<uint8_t>(type) << 4) | (token.size() & 0x0F);
    msg.push_back(header);

    // Code
    msg.push_back(static_cast<uint8_t>(method));

    // MID
    msg.push_back(static_cast<uint8_t>((mid >> 8) & 0xFF));
    msg.push_back(static_cast<uint8_t>(mid & 0xFF));

    // Token
    msg.insert(msg.end(), token.begin(), token.end());

    // Options

    uint16_t lastOpt = 0;

    // Observe option
    if (includeObserve) {
        encodeUintOption(msg, 6, observeValue, lastOpt);   // delta = 6 - 0 = 6
    }

    // URI-Path options
    for (const auto& segment : uriSegments) {
        encodeOption(msg, 11, segment, lastOpt); // Uri-Path = 11
    }

    // Content-Format option
    if (contentFormat != Format::NONE) {
        encodeUintOption(msg, 12, (uint16_t)contentFormat, lastOpt); // Content-Format = 12
    }

    // Payload
    if (method == Method::POST && !payload.empty()) {
        msg.push_back(0xFF); // Payload marker
        msg.insert(msg.end(), payload.begin(), payload.end());
    }

    return msg;
}


Coap::CoapReply Coap::parseObserveReply(const uint8_t* buffer, size_t len) {
    CoapReply reply;

    if (!Coap::looksLikeCoap(buffer, len)) {
        spdlog::warn("Invalid CoAP message received");
        return reply;
    }

    // MID
    reply.mid = Coap::getMid(buffer);

    // Token
    uint8_t tkl = Coap::getTokenLength(buffer);
    reply.token.insert(reply.token.end(), buffer + 4, buffer + 4 + tkl);

    // Locate payload marker (0xFF)
    size_t index = 4 + tkl;
    while (index < len && buffer[index] != 0xFF) {
        // Skip options (simplified: assumes well-formed)
        uint8_t optByte = buffer[index++];
        uint8_t optLen = optByte & 0x0F;
        index += optLen;
    }

    if (index < len && buffer[index] == 0xFF) {
        ++index; // skip marker
        reply.payload.insert(reply.payload.end(), buffer + index, buffer + len);
    }

    return reply;
}
