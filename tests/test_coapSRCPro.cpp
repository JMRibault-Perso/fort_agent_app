#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/crc.hpp>
#include <gtest/gtest.h>

#include <fort_agent/coapHelpers.h>
#include <fort_agent/coapPortTracker.h>

namespace {

std::vector<uint8_t> makeRequestFrame(uint16_t mid) {
    using namespace Coap;
    std::vector<std::string> segments{"st", "joystick", "combined"};
    return buildMessage(Type::CON, Method::GET, mid, segments, {}, {}, Format::NONE, false, 0);
}

uint8_t expectedMarker(uint16_t port) {
    const uint8_t portBytes[2]{static_cast<uint8_t>(port & 0xFF), static_cast<uint8_t>((port >> 8) & 0xFF)};
    boost::crc_optimal<8, 0x07, 0x00, 0x00, false, false> crc;
    crc.process_bytes(portBytes, 2);
    return crc.checksum();
}

TEST(CoapPortTrackerTest, UdpToSerialAddsTrackingTokensForRequests) {
    boost::asio::io_service service;
    CoapPortTracker tracker(service);
    const uint16_t port = 4321;
    auto frame = makeRequestFrame(0x2222);

    std::array<uint8_t, 256> buffer{};
    std::copy(frame.begin(), frame.end(), buffer.begin());
    size_t len = frame.size();
    tracker.udpToSerial(port, buffer.data(), &len, buffer.size());

    EXPECT_EQ(Coap::getTokenLength(buffer.data()), 3);
    EXPECT_EQ(len, frame.size() + 3);
    EXPECT_EQ(buffer[Coap::COAP_TOKEN_START_INDEX], expectedMarker(port));
    EXPECT_EQ(buffer[Coap::COAP_TOKEN_START_INDEX + 1], static_cast<uint8_t>(port & 0xFF));
    EXPECT_EQ(buffer[Coap::COAP_TOKEN_START_INDEX + 2], static_cast<uint8_t>((port >> 8) & 0xFF));
}

TEST(CoapPortTrackerTest, SerialToUdpStripsTrackingTokens) {
    boost::asio::io_service service;
    CoapPortTracker tracker(service);
    const uint16_t port = 7777;
    auto frame = makeRequestFrame(0x3333);

    std::array<uint8_t, 256> buffer{};
    std::copy(frame.begin(), frame.end(), buffer.begin());
    size_t len = frame.size();
    tracker.udpToSerial(port, buffer.data(), &len, buffer.size());

    std::array<uint8_t, 256> response{};
    std::copy(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(len), response.begin());
    size_t responseLen = len;
    response[0] = static_cast<uint8_t>((response[0] & 0x0F) | 0x40 | (static_cast<uint8_t>(Coap::Type::ACK) << 4));
    response[1] = 0x45; // 2.05 Content

    const uint16_t routedPort = tracker.serialToUdp(response.data(), &responseLen);
    EXPECT_EQ(routedPort, port);
    EXPECT_EQ(Coap::getTokenLength(response.data()), 0);
    EXPECT_EQ(responseLen, len - 3);
}

TEST(CoapPortTrackerTest, SerialToUdpUsesTrackedMidWhenTokensMissing) {
    boost::asio::io_service service;
    CoapPortTracker tracker(service);
    const uint16_t port = 9001;
    const uint16_t mid = 0x4444;
    auto frame = makeRequestFrame(mid);

    std::array<uint8_t, 256> buffer{};
    std::copy(frame.begin(), frame.end(), buffer.begin());
    size_t len = frame.size();
    tracker.udpToSerial(port, buffer.data(), &len, buffer.size());

    std::vector<uint8_t> response(frame);
    response[0] = 0x60; // ver=1, type=ACK, TKL=0
    response[1] = 0x45;
    size_t responseLen = response.size();

    const uint16_t routedPort = tracker.serialToUdp(response.data(), &responseLen);
    EXPECT_EQ(routedPort, port);
    EXPECT_EQ(responseLen, response.size());
}

TEST(CoapPortTrackerTest, SerialToUdpFallsBackToDefaultWhenUnmapped) {
    boost::asio::io_service service;
    CoapPortTracker tracker(service);
    auto frame = makeRequestFrame(0x5555);

    std::vector<uint8_t> response(frame);
    response[0] = 0x60;
    response[1] = 0x45;
    size_t responseLen = response.size();

    const uint16_t routedPort = tracker.serialToUdp(response.data(), &responseLen);
    EXPECT_EQ(routedPort, 5683);
}

TEST(CoapPortTrackerTest, ExtractUriPathConcatenatesSegments) {
    boost::asio::io_service service;
    CoapPortTracker tracker(service);
    auto frame = makeRequestFrame(0x6666);

    const std::string path = tracker.extractUriPath(frame.data(), frame.size());
    EXPECT_EQ(path, "st/joystick/combined");
}

}  // namespace
