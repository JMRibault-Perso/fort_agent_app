#include <fort_agent/slip.h>

#include <stddef.h>
#include <assert.h>

namespace FortAgentSlip {

    uint32_t
    createSlipEncodedMessage(uint8_t *dataToBeSent, const uint8_t *const data,
                             uint32_t length) {
        uint16_t slipDataLength = 0;

        // We are supporting MAX_PDU_FRAME_LEN frame size only
        if (length > FORT_AGENT_BUFFER_UNIT_SZ) {
            return 0;
        }

        dataToBeSent[slipDataLength++] = SLIP_SPECIAL_BYTE_END;
        for (int i = 0; i < length; i++) {
            if (data[i] == SLIP_SPECIAL_BYTE_END) {
                dataToBeSent[slipDataLength++] = SLIP_SPECIAL_BYTE_ESC;
                dataToBeSent[slipDataLength++] = SLIP_ESCAPED_BYTE_END;
            } else if (data[i] == SLIP_SPECIAL_BYTE_ESC) {
                dataToBeSent[slipDataLength++] = SLIP_SPECIAL_BYTE_ESC;
                dataToBeSent[slipDataLength++] = SLIP_ESCAPED_BYTE_ESC;
            } else {
                dataToBeSent[slipDataLength++] = data[i];
            }
        }

        dataToBeSent[slipDataLength++] = SLIP_SPECIAL_BYTE_END;

        return slipDataLength;
    }

    // https://github.com/marcinbor85/slip

    static void reset_rx(slip_handler_s *slip) {
        assert(slip != NULL);

        slip->state = SLIP_STATE_NORMAL;
        slip->size = 0;
    }

    slip_error_t
    slip_init(slip_handler_s *slip, const slip_descriptor_s *descriptor) {
        assert(slip != NULL);
        assert(descriptor != NULL);
        assert(descriptor->buf != NULL);
        assert(descriptor->recv_message != NULL);
        assert(descriptor->write_byte != NULL);

        slip->descriptor = descriptor;
        reset_rx(slip);

        return SLIP_NO_ERROR;
    }

    static slip_error_t put_byte_to_buffer(slip_handler_s *slip, uint8_t byte) {
        slip_error_t error = SLIP_NO_ERROR;

        assert(slip != NULL);

        if (slip->size >= slip->descriptor->buf_size) {
            error = SLIP_ERROR_BUFFER_OVERFLOW;
            reset_rx(slip);
        } else {
            slip->descriptor->buf[slip->size++] = byte;
            slip->state = SLIP_STATE_NORMAL;
        }

        return error;
    }

    slip_error_t slip_read_byte(slip_handler_s *slip, uint8_t byte) {
        slip_error_t error = SLIP_NO_ERROR;

        assert(slip != NULL);

        switch (slip->state) {
            case SLIP_STATE_NORMAL:
                switch (byte) {
                    case SLIP_SPECIAL_BYTE_END:
                        if (slip->size >= 2) {
                            slip->descriptor->recv_message(
                                slip->descriptor->buf,
                                slip->size
                            );
                        }
                        reset_rx(slip);
                        break;
                    case SLIP_SPECIAL_BYTE_ESC:
                        slip->state = SLIP_STATE_ESCAPED;
                        break;
                    default:
                        error = put_byte_to_buffer(slip, byte);
                        break;
                }
                break;

            case SLIP_STATE_ESCAPED:
                switch (byte) {
                    case SLIP_ESCAPED_BYTE_END:
                        byte = SLIP_SPECIAL_BYTE_END;
                        break;
                    case SLIP_ESCAPED_BYTE_ESC:
                        byte = SLIP_SPECIAL_BYTE_ESC;
                        break;
                    default:
                        error = SLIP_ERROR_UNKNOWN_ESCAPED_BYTE;
                        reset_rx(slip);
                        break;
                }

                if (error != SLIP_NO_ERROR)
                    break;

                error = put_byte_to_buffer(slip, byte);
                break;
        }

        return error;
    }

    static uint8_t write_encoded_byte(slip_handler_s *slip, uint8_t byte) {
        uint8_t escape = 0;

        assert(slip != NULL);

        switch (byte) {
            case SLIP_SPECIAL_BYTE_END:
                byte = SLIP_ESCAPED_BYTE_END;
                escape = 1;
                break;
            case SLIP_SPECIAL_BYTE_ESC:
                byte = SLIP_ESCAPED_BYTE_ESC;
                escape = 1;
                break;
        }

        if (escape != 0) {
            if (slip->descriptor->write_byte(SLIP_SPECIAL_BYTE_ESC) == 0)
                return 0;
        }
        if (slip->descriptor->write_byte(byte) == 0)
            return 0;

        return 1;
    }

    slip_error_t
    slip_send_message(slip_handler_s *slip, const uint8_t *const data,
                      uint32_t size) {
        uint32_t i;
        uint8_t byte;

        assert(data != NULL);
        assert(slip != NULL);

        if (slip->descriptor->write_byte(SLIP_SPECIAL_BYTE_END) == 0)
            return SLIP_ERROR_BUFFER_OVERFLOW;

        for (i = 0; i < size; i++) {
            byte = data[i];
            if (write_encoded_byte(slip, byte) == 0)
                return SLIP_ERROR_BUFFER_OVERFLOW;
        }

        if (slip->descriptor->write_byte(SLIP_SPECIAL_BYTE_END) == 0)
            return SLIP_ERROR_BUFFER_OVERFLOW;

        return SLIP_NO_ERROR;
    }
}