#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <fort_agent/coapHelpers.h>

namespace {

TEST(CoapHelpersTest, BuildMessageEncodesHeadersOptionsAndPayload) {
    using namespace Coap;
    const std::vector<std::string> uri{"foo", "bar"};
    const std::vector<uint8_t> payload{0xDE, 0xAD};
    const std::vector<uint8_t> token{0xAA, 0xBB};

    auto msg = buildMessage(Type::CON, Method::POST, 0x1234, uri, payload, token,
                            Format::APPLICATION_CBOR, true, 0x01);

    ASSERT_GE(msg.size(), 4u + token.size());
    EXPECT_EQ(msg[0], static_cast<uint8_t>(0x40 | (static_cast<uint8_t>(Type::CON) << 4) | token.size()));
    EXPECT_EQ(msg[1], static_cast<uint8_t>(Method::POST));
    EXPECT_EQ(msg[2], 0x12);
    EXPECT_EQ(msg[3], 0x34);
    ASSERT_GE(msg.size(), 6u);
    EXPECT_EQ(std::vector<uint8_t>(msg.begin() + 4, msg.begin() + 6), token);

    const std::vector<uint8_t> expectedOptions{
        0x61, 0x01,                                    // Observe option value 1
        0x53, 'f', 'o', 'o',                          // Uri-Path "foo"
        0x03, 'b', 'a', 'r',                          // Uri-Path "bar"
        0x11, static_cast<uint8_t>(Format::APPLICATION_CBOR)  // Content-Format
    };
    const size_t optionsOffset = 4 + token.size();
    ASSERT_LE(optionsOffset + expectedOptions.size(), msg.size());
    EXPECT_EQ(std::vector<uint8_t>(msg.begin() + optionsOffset,
                                   msg.begin() + optionsOffset + expectedOptions.size()),
              expectedOptions);

    const size_t payloadMarkerIndex = optionsOffset + expectedOptions.size();
    ASSERT_LT(payloadMarkerIndex, msg.size());
    EXPECT_EQ(msg[payloadMarkerIndex], 0xFF);
    EXPECT_EQ(std::vector<uint8_t>(msg.begin() + payloadMarkerIndex + 1, msg.end()), payload);
}

TEST(CoapHelpersTest, BuildMessageSkipsPayloadMarkerForEmptyPayload) {
    using namespace Coap;
    auto msg = buildMessage(Type::CON, Method::GET, 0x0101, {"alpha"}, {}, {},
                            Format::NONE, false, 0);

    EXPECT_EQ(Coap::getTokenLength(msg.data()), 0);
    EXPECT_EQ(std::find(msg.begin(), msg.end(), static_cast<uint8_t>(0xFF)), msg.end());
}

TEST(CoapHelpersTest, SetTokenLengthRejectsValuesGreaterThanEight) {
    uint8_t buffer[4]{0x40, 0x01, 0x00, 0x00};
    EXPECT_THROW(Coap::setTokenLength(buffer, 9), CoapException);
}

TEST(CoapHelpersTest, ParseObserveReplyExtractsMidTokenAndPayload) {
    using namespace Coap;
    const std::vector<std::string> uri{"foo"};
    const std::vector<uint8_t> token{0xFE, 0xED};
    const std::vector<uint8_t> payload{0x10, 0x20, 0x30};

    auto msg = buildMessage(Type::ACK, Method::POST, 0xABCD, uri, payload, token,
                            Format::TEXT_PLAIN, true, 0);
    msg[1] = 0x45; // 2.05 Content

    auto reply = parseObserveReply(msg.data(), msg.size());
    EXPECT_EQ(reply.mid, 0xABCD);
    EXPECT_EQ(reply.token, token);
    EXPECT_EQ(reply.payload, payload);
}

TEST(CoapHelpersTest, LooksLikeCoapRejectsInvalidVersion) {
    using namespace Coap;
    auto msg = buildMessage(Type::CON, Method::GET, 0x0202, {"foo"}, {}, {},
                            Format::NONE, false, 0);
    msg[0] &= 0x3F; // zero out version bits

    EXPECT_FALSE(Coap::looksLikeCoap(msg.data(), msg.size()));
}

TEST(CoapHelpersTest, CreateResetMsgProducesExpectedLayout) {
    const auto reset = Coap::createResetMsg(0xA1B2);
    std::array<uint8_t, 4> expected{0x70, 0x00, 0xA1, 0xB2};
    EXPECT_EQ(reset, expected);
}

}  // namespace
