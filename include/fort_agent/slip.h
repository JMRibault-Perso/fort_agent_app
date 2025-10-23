#ifndef FORT_AGENT_CPP_SLIP_H
#define FORT_AGENT_CPP_SLIP_H

#include <cstdint>
#include <functional>

#include <fort_agent/dbgTrace.h>

#define FORT_AGENT_BUFFER_UNIT_SZ 512

namespace FortAgentSlip {
    uint32_t
    createSlipEncodedMessage(uint8_t *dataToBeSent, const uint8_t *const data,
                             uint32_t length);
    // https://github.com/marcinbor85/slip

#define SLIP_SPECIAL_BYTE_END           0xC0
#define SLIP_SPECIAL_BYTE_ESC           0xDB

#define SLIP_ESCAPED_BYTE_END           0xDC
#define SLIP_ESCAPED_BYTE_ESC           0xDD

    typedef enum {
        SLIP_STATE_NORMAL = 0x00,
        SLIP_STATE_ESCAPED
    } slip_state_t;

    typedef enum {
        SLIP_NO_ERROR = 0x00,
        SLIP_ERROR_BUFFER_OVERFLOW,
        SLIP_ERROR_UNKNOWN_ESCAPED_BYTE,
    } slip_error_t;

    typedef struct {
        uint8_t *buf;
        uint32_t buf_size;
        // void (*recv_message)(uint8_t *data, uint32_t size);
        std::function<void(uint8_t *, uint32_t)> recv_message;
        // uint8_t (*write_byte)(uint8_t byte);
        std::function<uint8_t(uint8_t)> write_byte;
    } slip_descriptor_s;

    typedef struct {
        slip_state_t state;
        uint32_t size;
        uint16_t crc;
        const slip_descriptor_s *descriptor;
    } slip_handler_s;

    slip_error_t
    slip_init(slip_handler_s *slip, const slip_descriptor_s *descriptor);

    slip_error_t slip_read_byte(slip_handler_s *slip, uint8_t byte);

    slip_error_t
    slip_send_message(slip_handler_s *slip, const uint8_t *const data,
                      uint32_t size);
}

#endif //FORT_AGENT_CPP_SLIP_H
