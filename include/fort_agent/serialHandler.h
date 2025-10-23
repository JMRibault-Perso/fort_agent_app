#ifndef FORT_AGENT_SERIALHANDLER_H
#define FORT_AGENT_SERIALHANDLER_H

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/circular_buffer.hpp>

#include <fort_agent/dbgTrace.h>
#include <fort_agent/slip.h>


enum class State {
    RESETTING,
    OPERATIONAL,
    OPERATIONAL_ERROR,
    MAX_STATES
};

typedef std::function<void(const std::string &)> errorCb;
typedef std::function<void(uint8_t *, uint32_t)> dataCb;

class SerialHandler {
public:
    SerialHandler(boost::asio::io_service &ioService,
                  const std::string &serialPath,
                  errorCb onFailure,
                  dataCb onData);

    bool asyncSendMessageToSerialPort(const void *dataBytes, size_t dataLen);

private:

    boost::asio::io_service &service;
    boost::asio::serial_port serial;
    static constexpr int baudRate = 115200;
    static constexpr boost::asio::serial_port_base::flow_control::type flowControl = boost::asio::serial_port_base::flow_control::none;

    State state;

    std::array<uint8_t, FORT_AGENT_BUFFER_UNIT_SZ> readTempBuffer;
    boost::circular_buffer<uint8_t> readBuffer;
    boost::circular_buffer<uint8_t> writeBuffer;
    bool writeInProgress;

    // Callbacks
    errorCb onFailure;
    dataCb onData;

    // Operational
    void enterOperationalState();

    void listenToSerialPort();

    void operationalFailure(const std::string &errMsg);

    /** Returns number of bytes that were buffered to send.
     *  Only returns nonzero when in OPERATIONAL state.
     *  If in OPERATIONAL state and asyncSendDataToSerialPort returns less than len, that means its internal
     *  buffer is full and that's generally a bad sign.  Either way too much data is being stuffed
     *  into the serial port, or the serial connection has stopped working.
     */
    size_t asyncSendDataToSerialPort(const void *data, size_t len);

    void asyncWriteToSerialPortCb(const boost::system::error_code &ec,
                                  size_t bytes_transferred);

    static constexpr size_t RX_BUFFER_SIZE = FORT_AGENT_BUFFER_UNIT_SZ;
    static constexpr size_t TX_BUFFER_SIZE = FORT_AGENT_BUFFER_UNIT_SZ;

    FortAgentSlip::slip_descriptor_s slip_desc;
    FortAgentSlip::slip_handler_s slip_handle;
    uint8_t slip_buffer[FORT_AGENT_BUFFER_UNIT_SZ];


};

#endif //FORT_AGENT_SERIALHANDLER_H
