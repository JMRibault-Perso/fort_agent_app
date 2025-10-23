#ifndef FORT_AGENT_CPP_COAPPORTTRACKER_H
#define FORT_AGENT_CPP_COAPPORTTRACKER_H

#include <cstdint>

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <boost/asio.hpp>

#include <fort_agent/dbgTrace.h>

class MappedMid {
public:
    uint16_t port;
    std::chrono::time_point<std::chrono::steady_clock> timeCreated;
};

/* Port tracking works as follows:
 *
 * For CoAP going from local UDP to serial:
 * - If the code is 0.00 (Empty message), map MID to port and send as-is
 * - If the token length is more than 5, map MID to port and send as-is (can't add token tracking)
 * - Otherwise, prepend a marker byte followed by the port (little-endian) to the tokens
 *
 * For CoAP going from serial to local UDP:
 * - If the code is 0.00, or there are fewer than 3 tokens, or the first token is not the marker
 *   - If the MID is in the map, send to that port
 *   - Otherwise, send to default CoAP server port
 * - Otherwise (message is non-Empty with token merker present), extract port from token and send
 *   - Marker and port tokens are removed from the message after being extracted
 *
 * Port/MID mappings can be overwritten by newer messages with the same MID but different port, and
 * expire after midTImeoutTime.
 */

class CoapPortTracker {
public:
    CoapPortTracker(boost::asio::io_service &service,
                    uint16_t defaultCoapPort = 5683);

    ~CoapPortTracker() = default;

    // udpToSerial and serialToUdp will throw a CoapException if the coapFrameBuffer is not valid CoAP

    // add port info to CoAP token or track MID, depending on message type
    void udpToSerial(uint16_t port, void *buffer, size_t *len, size_t maxLen);

    // extract port info or retrieve mapped port from MID, depending on message type
    uint16_t serialToUdp(void *buffer, size_t *len);
    std::string extractUriPath(const uint8_t* data, size_t size);

    void clear();

private:
    const uint16_t defaultPort;

    // Token tracking
    static uint8_t markerByte(const uint8_t *portBytes);

    // MID tracking
    void removeOldMids();

    void trackMid(uint16_t port, uint16_t mid);

    std::map<uint16_t, MappedMid> mids;
    boost::asio::steady_timer midExpirationTimer;

    // Debug helper
    static std::string dataToString(const void *data, size_t len);
};

#endif //FORT_AGENT_CPP_COAPPORTTRACKER_H
