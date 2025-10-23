#include <fort_agent/serialHandler.h>
#include <spdlog/spdlog.h>

uint8_t FAUX_write_byte(uint8_t byte) {
    throw std::runtime_error("Don't use slip_desc to to write");
}

SerialHandler::SerialHandler(boost::asio::io_service &ioService,
                             const std::string &serialPath,
                             errorCb onFailure,
                             dataCb onDataCb) :
    service(ioService),
    serial(ioService),
    readTempBuffer(),
    readBuffer(RX_BUFFER_SIZE),
    writeBuffer(TX_BUFFER_SIZE),
    writeInProgress(false),
    onFailure(onFailure),
    onData(onDataCb) {
    FXN_TRACE;
    try {
        state = State::RESETTING;

        serial.open(serialPath);
        serial.set_option(boost::asio::serial_port_base::baud_rate(baudRate));
        serial.set_option(
            boost::asio::serial_port_base::flow_control(flowControl));


        slip_desc.buf = slip_buffer;
        slip_desc.buf_size = FORT_AGENT_BUFFER_UNIT_SZ;
        slip_desc.recv_message = onData;
        slip_desc.write_byte = FAUX_write_byte;

        FortAgentSlip::slip_error_t res;
        res = slip_init(&slip_handle, &slip_desc);
        if (res != FortAgentSlip::SLIP_NO_ERROR) {
            throw std::runtime_error("Failed to initialze slip interface = " +
                                     res);
        }
        enterOperationalState();


    }
    catch (std::exception &e) {
        throw std::runtime_error(
            "Failed to open/configure serial port at " + serialPath + " exception: " + e.what() );
    }

}

bool SerialHandler::asyncSendMessageToSerialPort(const void *dataBytes,
                                                 size_t dataLen) {
    FXN_TRACE;
    // don't send if serial port has not been fully initialized
    if (state != State::OPERATIONAL) {
        spdlog::error("Failed sending message, state is {}", state);
        return false;
    }

    // don't bother trying to send if there's not enough free space in the buffer for the whole command
    if (writeBuffer.reserve() < dataLen) {
        spdlog::error(
            "Can't send data message: not enough free write buffer space");
        return false;
    }

    // larger messages will need to be split across multiple data commands
    while (dataLen > 0) {
        const auto toSend = std::min((size_t) FORT_AGENT_BUFFER_UNIT_SZ,
                                     dataLen);
        uint8_t cmd[FORT_AGENT_BUFFER_UNIT_SZ];
        const uint32_t cmdLen = FortAgentSlip::createSlipEncodedMessage(
            reinterpret_cast<uint8_t *>(&cmd),
            reinterpret_cast<const uint8_t *const>(dataBytes),
            static_cast<uint32_t>(toSend));
        if (cmdLen == 0) {
            spdlog::error(
                "Can't send data message: creating data command failed");
            return false;
        } else {
            asyncSendDataToSerialPort(&cmd, cmdLen);
            dataLen -= toSend;
        }
    }

    return true;
}

size_t
SerialHandler::asyncSendDataToSerialPort(const void *sendData, size_t len) {
    FXN_TRACE;
    if (state != State::OPERATIONAL) {
        return 0;
    }

    const uint8_t *data = reinterpret_cast<const uint8_t *>(sendData);
    size_t written;
    for (written = 0; written < len && !writeBuffer.full(); written++) {
        writeBuffer.push_back(data[written]);
    }

    // begin a new async serial write if no write is already in progress
    if (!writeInProgress) {
        writeInProgress = true;

        // send the first contiguous memory region of the circular buffer out the serial port
        const auto &range = writeBuffer.array_one();
        auto toWrite = boost::asio::buffer(range.first, range.second);

        serial.async_write_some(toWrite, std::bind(
            &SerialHandler::asyncWriteToSerialPortCb, this,
            std::placeholders::_1, std::placeholders::_2));
    }

    return written;
}

void
SerialHandler::asyncWriteToSerialPortCb(const boost::system::error_code &ec,
                                        size_t bytes_transferred) {
    FXN_TRACE;
    if (ec) {
        spdlog::debug("async send data error: {}", ec.message());
        writeInProgress = false;
    } else {
        // spdlog::debug("Wrote {} bytes to serial port", bytes_transferred);

        writeBuffer.erase_begin(bytes_transferred);
        if (writeBuffer.empty()) {
            writeInProgress = false;
        } else {  // kick off another async write for the remaining data
            const auto &range = writeBuffer.array_one();
            auto toWrite = boost::asio::buffer(range.first, range.second);
            serial.async_write_some(toWrite, std::bind(
                &SerialHandler::asyncWriteToSerialPortCb, this,
                std::placeholders::_1, std::placeholders::_2));
        }
    }
}

void SerialHandler::enterOperationalState() {
    FXN_TRACE;
    state = State::OPERATIONAL;
    spdlog::info("Serial port now fully configured and accepting data");

    // begin listening
    listenToSerialPort();
}

void SerialHandler::listenToSerialPort() {
    FXN_TRACE;
    serial.async_read_some(
        boost::asio::buffer(readTempBuffer.data(), readTempBuffer.size()),
        [this](const boost::system::error_code &ec, size_t bytes_transferred) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    spdlog::error("Operational mode serial listener cancelled");
                    // do nothing but log cancellation
                } else {
                    operationalFailure(
                        fmt::format("Error reading serial port: {}",
                                    ec.message()));
                }
            } else {
                // spdlog::debug("Got data message ({} Bytes) from serial = {}", bytes_transferred, readTempBuffer.data());


                // Push all bytes of recv message into the circular buffer
                for (int byte = 0;
                     byte < bytes_transferred && !readBuffer.full(); byte++) {
                    readBuffer.push_back(readTempBuffer.at(byte));
                }

                while (!readBuffer.empty()) {
                    // slip_read_byte will issue a call back when a complete
                    // slip message has been read from readBuffer, recv_message
                    slip_read_byte(&slip_handle, readBuffer.front());
                    readBuffer.pop_front();
                }

                // continue listening so long as the state is OPERATIONAL
                if (state == State::OPERATIONAL) {
                    listenToSerialPort();
                }
            }
        }
    );
}


void SerialHandler::operationalFailure(const std::string &errMsg) {
    FXN_TRACE;
    state = State::OPERATIONAL_ERROR;
    onFailure(errMsg);
}

