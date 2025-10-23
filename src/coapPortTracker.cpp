#include <fort_agent/coapPortTracker.h>

#include <sstream>

#include <boost/crc.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <fort_agent/coapHelpers.h>

using fmt::format;

#include <string>
#include <vector>
#include <sstream>

std::string CoapPortTracker::extractUriPath(const uint8_t* data, size_t size) {
    if (size < 4) return "";

    uint8_t ver_type_token = data[0];
    uint8_t token_length = ver_type_token & 0x0F;
    size_t pos = 4 + token_length;  // Skip header + token

    uint16_t last_option_number = 0;
    std::vector<std::string> path_segments;

    while (pos < size && data[pos] != 0xFF) {  // 0xFF = payload marker
        uint8_t delta = (data[pos] & 0xF0) >> 4;
        uint8_t length = (data[pos] & 0x0F);
        pos++;

        if (delta == 13) {
            delta = 13 + data[pos++];
        } else if (delta == 14) {
            delta = 269 + (data[pos] << 8) + data[pos + 1];
            pos += 2;
        }

        if (length == 13) {
            length = 13 + data[pos++];
        } else if (length == 14) {
            length = 269 + (data[pos] << 8) + data[pos + 1];
            pos += 2;
        }

        last_option_number += delta;

        if (last_option_number == 11) {  // Uri-Path
            std::string segment(reinterpret_cast<const char*>(&data[pos]), length);
            path_segments.push_back(segment);
        }

        pos += length;
    }

    std::ostringstream uri;
    for (size_t i = 0; i < path_segments.size(); ++i) {
        if (i > 0) uri << "/";
        uri << path_segments[i];
    }

    return uri.str();  // e.g., "st/joystick/combined"
}

CoapPortTracker::CoapPortTracker(boost::asio::io_service &service,
                                 uint16_t defaultCoapPort) :
    defaultPort(defaultCoapPort),
    mids(),
    midExpirationTimer(service) {
    // nothing to remove yet, but calling this will start the timer
    removeOldMids();
}

void CoapPortTracker::removeOldMids() {
    const auto now = std::chrono::steady_clock::now();

    for (auto it = mids.begin();
         it != mids.end(); /* no auto-increment here */ ) {
        if (now - it->second.timeCreated >= Coap::midTimeoutTime) {
            spdlog::debug("Erasing old tracked MID {} : port {}", it->first,
                          it->second.port);
            it = mids.erase(it);
        } else {
            it++;
        }
    }

    midExpirationTimer.expires_from_now(std::chrono::seconds(1));
    midExpirationTimer.async_wait([this](const boost::system::error_code &ec) {
        if (ec.value() == 125) {
            // operation cancelled
            return;
        }
        removeOldMids();
    });
}

void CoapPortTracker::trackMid(uint16_t port, uint16_t mid) {
    const auto now = std::chrono::steady_clock::now();

    auto it = mids.find(mid);
    if (it ==
        mids.end()) {  // no association, map mid to port and insertion time
        mids.insert({mid, {port, now}});
    } else {
        if (it->second.port ==
            port) {  // association already exists, update timestamp
            it->second.timeCreated = now;
        } else {  // association already exists and the port is different, overwrite but display
            spdlog::debug(
                "Overwriting existing association of MID {} : port {} with new port {}",
                mid, it->second.port, port);
            it->second = {port, now};
        }
    }
}

std::string CoapPortTracker::dataToString(const void *data, size_t len) {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
    for (size_t i = 0; i < len; i++) {
        if (i != 0) {
            ss << " ";
        }
        if ((bytes[i] >= 'a' && bytes[i] <= 'z') ||
            (bytes[i] >= 'A' && bytes[i] <= 'Z') ||
            (bytes[i] >= '0' && bytes[i] <= '9')) {
            ss << "'" << static_cast<char>(bytes[i]) << "'";
        } else {
            ss << "0x" << static_cast<int>(bytes[i]);
        }
    }
    return ss.str();
}

uint8_t CoapPortTracker::markerByte(const uint8_t *portBytes) {
    boost::crc_optimal<8, 0x07, 0x00, 0x00, false, false> crc8;
    crc8.process_bytes(portBytes, 2);
    return crc8.checksum();
}

/* Message tracking rules:
 *   Outbound:
 *     Request: Add tracking tokens when possible, track MID when Confirmable
 *     Response: Track MID when Confirmable
 *     Empty: Track MID
 *     Other: Log error and don't send
 *   Inbound:
 *     Request: Don't modify, send to server port
 *     Response: Extract tracking tokens if possible, otherwise use tracked MID if possible, otherwise send to server port
 *     Empty: Use tracked MID if possible, otherwise send to server port
 *     Other: Log error and don't send
 */

void CoapPortTracker::udpToSerial(uint16_t port, void *buffer, size_t *len,
                                  size_t maxLen) {
    uint8_t *coapFrameBuffer = static_cast<uint8_t *>(buffer);

    if (maxLen < *len) {
        throw CoapException(
            format("maxLen ({}) less than buffer length ({})", maxLen, *len));
    }
    if (!Coap::looksLikeCoap(coapFrameBuffer, *len)) {
        throw CoapException("Packet from UDP is not valid CoAP");
    }

    const size_t tokenLength = Coap::getTokenLength(coapFrameBuffer);

    if (Coap::isRequest(coapFrameBuffer)) {
        COUNT_REQUEST_TO_NSC;
        if (tokenLength < 5 &&
            maxLen - *len >= 3) {  // room for tracking tokens
            uint8_t *const tokenPtr = &coapFrameBuffer[Coap::COAP_TOKEN_START_INDEX];

            // shift whole message up by three bytes, starting at the existing tokens (if any), to
            // create a gap for the three new tokens
            memmove(tokenPtr + 3, tokenPtr,
                    *len - Coap::COAP_TOKEN_START_INDEX);

            // insert port, little-endian
            *(tokenPtr + 1) = (port & 0xff);
            *(tokenPtr + 2) = ((port >> 8) & 0xff);

            // create a marker token from the two port bytes
            *tokenPtr = markerByte(tokenPtr + 1);

            // update the token length
            Coap::setTokenLength(coapFrameBuffer, tokenLength + 3);

            // update the message length
            (*len) += 3;

            // track the outgoing MID when the message is marked CON
            if (Coap::getCoapType(coapFrameBuffer) == Coap::Type::CON) {
                trackMid(port, Coap::getMid(coapFrameBuffer));
                spdlog::debug(
                    "UDP: Adding tracking tokens to message from port {} and tracking MID {}, msg = {}",
                    port, Coap::getMid(coapFrameBuffer),
                    dataToString(buffer, *len));
            } else {
                spdlog::debug(
                    "UDP: Adding tracking tokens to message from port {}, msg = {}",
                    port, dataToString(buffer, *len));
            }
        } else {
            trackMid(port, Coap::getMid(coapFrameBuffer));
            spdlog::debug("UDP: Tracking MID {} -> port {}, msg = {}",
                          Coap::getMid(coapFrameBuffer), port,
                          dataToString(buffer, *len));
        }
    } else if (Coap::isResponse(coapFrameBuffer)) {
        COUNT_RESPONSE_FROM_EPC;
        // track the outgoing MID when the message is marked CON
        if (Coap::getCoapType(coapFrameBuffer) == Coap::Type::CON) {
            trackMid(port, Coap::getMid(coapFrameBuffer));
            spdlog::debug("UDP: Tracking response MID {} -> port {}, msg = {}",
                          Coap::getMid(coapFrameBuffer), port,
                          dataToString(buffer, *len));
        } else {
            // do nothing
            spdlog::debug("UDP: Not tracking response from port {}, msg = {}",
                          port, dataToString(buffer, *len));
        }
    } else if (Coap::isEmpty(coapFrameBuffer)) {
        trackMid(port, Coap::getMid(coapFrameBuffer));
        spdlog::debug("UDP: Tracking MID {} -> port {}, msg = {}",
                      Coap::getMid(coapFrameBuffer), port,
                      dataToString(buffer, *len));
    } else {
        spdlog::debug(
            "UDP: Rejecting message, not request/response/empty, port {}, msg = {}",
            port, dataToString(buffer, *len));
    }
}

uint16_t CoapPortTracker::serialToUdp(void *buffer, size_t *len) {
    uint8_t *coapFrameBuffer = static_cast<uint8_t *>(buffer);

    if (!Coap::looksLikeCoap(coapFrameBuffer, *len)) {
        throw CoapException("Packet from serial port is not valid CoAP");
    }

    uint8_t *const firstToken = &coapFrameBuffer[Coap::COAP_TOKEN_START_INDEX];
    const uint8_t tokenLength = Coap::getTokenLength(coapFrameBuffer);

    if (Coap::isRequest(coapFrameBuffer)) {
        COUNT_REQUEST_TO_EPC;
        spdlog::debug("Serial: Forwarding CoAP request with MID {} to port {}",
                      Coap::getMid(coapFrameBuffer), defaultPort);
        return defaultPort;
    } else if (Coap::isResponse(coapFrameBuffer)) {
        COUNT_RESPONSE_FROM_NSC;
        if (tokenLength >= 3 && *firstToken == markerByte(
            firstToken + 1)) {  // valid tracking token found
            // extract port from token bytes
            const uint16_t port = static_cast<uint16_t>(*(firstToken + 1)) |
                                  (static_cast<uint16_t>(*(firstToken + 2))
                                      << 8);

            // shift the rest of the message down to overwrite the marker and port token bytes
            memmove(firstToken, firstToken + 3,
                    *len - (Coap::COAP_TOKEN_START_INDEX + 3));

            // update the token length
            Coap::setTokenLength(coapFrameBuffer, tokenLength - 3);

            // update the message length
            (*len) -= 3;

            spdlog::debug("Serial: Extracted port {} from tokens", port);
            return port;
        } else {  // no tracking token, check for tracked MID
            const auto it = mids.find(Coap::getMid(coapFrameBuffer));
            if (it ==
                mids.end()) {  // no mapping from MID to port, use default CoAP port and don't modify length
                spdlog::debug(
                    "Serial: No mapping for response with MID {}, sending to port {}",
                    Coap::getMid(coapFrameBuffer), defaultPort);
                return defaultPort;
            } else {  // mapping found, use the mapped port and don't modify length
                spdlog::debug(
                    "Serial: Got mapping for response with MID {}, sending to port {}",
                    it->first, it->second.port);
                return it->second.port;
            }
        }
    } else if (Coap::isEmpty(coapFrameBuffer)) {
        const auto it = mids.find(Coap::getMid(coapFrameBuffer));
        if (it ==
            mids.end()) {  // no mapping from MID to port, use default CoAP port and don't modify length
            spdlog::debug(
                "Serial: No mapping for Empty with MID {}, sending to port {}",
                Coap::getMid(coapFrameBuffer), defaultPort);
            return defaultPort;
        } else {  // mapping found, use the mapped port and don't modify length
            spdlog::debug(
                "Serial: Got mapping for Empty with MID {}, sending to port {}",
                it->first, it->second.port);
            return it->second.port;
        }
    } else {
        spdlog::debug(
            "Serial: Rejecting message, not request/response/empty, msg = {}",
            dataToString(buffer, *len));
        throw CoapException("Packet from serial is not valid CoAP");
    }
}

void CoapPortTracker::clear() {
    mids.clear();
}

