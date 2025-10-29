#include <fort_agent/uartCoapBridge.h>

#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <fort_agent/coapHelpers.h>
#include <fort_agent/coapJausBridge.h>

using fmt::format;

#include <iomanip>
#include <string>
#include <vector>
#include <sstream>


UartCoapBridge::UartCoapBridge(
    boost::asio::io_service &service,
    const std::string &localAddr,
    uint16_t localPort,
    const std::string &serialPath,
    const std::string &remoteAddr,
    uint16_t remotePort,
    const std::string &jausRemoteAddr,
    uint16_t jausRemotePort) :
    ioService(service),
    serialHandler(),
    listenPort(localPort),
    localHost(boost::asio::ip::address::from_string(localAddr)),
    localEndpoint(localHost, localPort),
    localSocket(service),
    localBindRetryTimer(service),
    from(),
    remoteHost(boost::asio::ip::address::from_string(remoteAddr)),
    jausHemoteHost(boost::asio::ip::address::from_string(jausRemoteAddr)),
    coapPorts(service, remotePort),
    failedToBindLocal(spdlog::level::err),
    failedToReceiveFromRemote(spdlog::level::err),
    failedToSendSerial(spdlog::level::warn),
    failedToListenSerial(spdlog::level::warn),
    failedToSendToRemote(),
    jausRemotePort(jausRemotePort) {
    FXN_TRACE;
    serialHandler = std::make_unique<SerialHandler>(
        service,
        serialPath,
        std::bind(&UartCoapBridge::serialError, this, std::placeholders::_1),
        std::bind(&UartCoapBridge::serialDataReceived, this,
                  std::placeholders::_1, std::placeholders::_2));

    // send Observe request for joystick/combined resource
    sendObserveJoystickCombinedRequest(1);
    sendObserveJoystickCombinedRequest(0);

    // send Observe request for SRCP Mode resource
    sendObserveSRCPModeRequest(1);
    //sendObserveSRCPModeRequest(0);

    // bind local socket to local port and begin listening
    bindLocal();
}

void UartCoapBridge::serialError(const std::string &errMsg) {
    FXN_TRACE;
    const std::string finalMsg = fmt::format("Serial port fatal error: {}",
                                             errMsg);
    spdlog::critical(finalMsg);
    throw std::runtime_error(finalMsg);  // will cause the application to exit
}

std::string UartCoapBridge::dataToHex(const void *data, size_t len) {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);

    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (size_t i = 0; i < len; i++) {
        if (i != 0) {
            ss << " ";
        }
        ss << "0x" << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

void UartCoapBridge::serialDataReceived(uint8_t *message, uint32_t size) {
    FXN_TRACE;
    if (size <= 0) {
        return;
    }

    // spdlog::debug("Serial data received, {} bytes payload", size);

    uint16_t port = 0;

    try {
        size_t len = size;
        std::shared_ptr<uint8_t[]> data(new uint8_t[len + 3]);
        
        std::copy_n(message, len, data.get());
        port = coapPorts.serialToUdp(data.get(), &len);

        // Special handling for JAUS messages - forward to JAUS bridge
        // The port range 900-1100 is reserved for JAUS messages
        if (port >= 900 && port <= 1100) {

            
            JausBridge JausBridge;
    
            spdlog::debug("JAUS: Tracking response MID {} -> port {}, msg = {}",
                Coap::getMid(data.get()), port,
                UartCoapBridge::dataToHex(data.get(), len));

                    size_t offset = 0;

            Coap::CoapReply reply = Coap::parseObserveReply(data.get(), len);

            if (JausBridge.EvaluateCoapJausMessage(port, reply.payload.data(), reply.payload.size())) {
                // Message was handled by JAUS bridge, do not forward
                spdlog::debug("JAUS: CoAP message handled by JAUS bridge, not forwarding");
                return;
            }

            switch (port)
            {
            case 1001: // SRCP Mode
                /* code */
                break;
            
            default:
                break;
            }
        } else {
            // Forward everything else to remote CoAP server
            sendToRemote(boost::asio::ip::udp::endpoint(remoteHost, port), data, len);
            clearFailedToSendToRemote(port);
        }
    }
    catch (CoapException &e) {
        logFailedToSendToRemote(port,
                                "Failed to forward received Serial data: Coap error: {}",
                                e.what());
    }

}

void UartCoapBridge::bindLocal() {
    FXN_TRACE;
    try {
        // close any existing bound socket
        localSocket.close();

        // create and bind the UDP socket
        localSocket = boost::asio::ip::udp::socket(ioService, localEndpoint);
        spdlog::info("Successfully bound to {}", localEndpoint);
        failedToBindLocal.clear();

        // begin listening for data
        receiveFromRemote();
    }
    catch (...) {
        failedToBindLocal.log("Failed to bind to {}", localEndpoint);

        // make sure the socket is closed
        if (localSocket.is_open()) {
            localSocket.close();
        }

        // schedule another attempt
        localBindRetryTimer.expires_from_now(
            boost::posix_time::seconds(localBindRetrySeconds));
        localBindRetryTimer.async_wait(
            [this](const boost::system::error_code & /*e*/) {
                bindLocal();
            });
    }
}

void UartCoapBridge::receiveFromRemote() {
    FXN_TRACE;
    try {
        std::shared_ptr<char[]> data(new char[FORT_AGENT_BUFFER_UNIT_SZ + 3]);

        /* Using a single 'from' variable should be OK, because only a single instance of
         * receive() is ever run at a time, so only once instance of async_receive_from() is ever
         * active per class.
         */
        localSocket.async_receive_from(
            boost::asio::buffer(data.get(), FORT_AGENT_BUFFER_UNIT_SZ),
            from,
            // remoteHost,
            [this, data](boost::system::error_code ec,
                         std::size_t bytes_recvd) {
                if (ec == boost::asio::error::operation_aborted) {
                    // operation cancelled
                    return;

                } else if (!ec && bytes_recvd > 0) {
                    if (from.address() == remoteHost) {
                        failedToReceiveFromRemote.clear();
                        const uint16_t port = from.port();

                        try {
                            size_t len = bytes_recvd;
                            coapPorts.udpToSerial(port, data.get(), &len,
                                                  FORT_AGENT_BUFFER_UNIT_SZ +
                                                  3);
                            serialHandler->asyncSendMessageToSerialPort(
                                data.get(),
                                len);
                        }
                        catch (CoapException &e) {
                            failedToSendSerial.log(
                                "Failed to send message to serial: {}",
                                e.what());
                        }
                    } else {
                        spdlog::trace(
                            "Received traffic from {} and not the desired remote {}",
                            from.address(), remoteHost);
                    }


                } else {
                    failedToListenSerial.log(
                        "Listen failure for {}, ec = {}, bytes_recvd = {}",
                        localEndpoint, ec, bytes_recvd);
                }

                receiveFromRemote();
            }
        );
    }
    catch (...) {
        spdlog::error("Bound socket receive failed");
        bindLocal();
    }
}

void UartCoapBridge::sendToRemote(boost::asio::ip::udp::endpoint to,
                                  std::shared_ptr<uint8_t[]> data,
                                  std::size_t length) {
    FXN_TRACE;
    if (!localSocket.is_open()) {
        logFailedToSendToRemote(to.port(), "Error sending to {}", to);
        return;
    }

    // spdlog::debug("Sending data to {}", to);

    localSocket.async_send_to(
        boost::asio::buffer(data.get(), length),
        to,
        [this, data, to](boost::system::error_code ec, std::size_t bytes_sent) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    // operation cancelled
                    return;
                } else {
                    logFailedToSendToRemote(to.port(),
                                            "Got error when sending to {}, ec = {}",
                                            to, ec);
                }
            } else {
                // sending successful
                // spdlog::debug("Sent {} to {}", bytes_sent, to);
                clearFailedToSendToRemote(to.port());
            }
        }
    );
}

template<typename FormatString, typename... Args>
void
UartCoapBridge::logFailedToSendToRemote(uint16_t port, const FormatString &fmt,
                                        Args &&...args) {
    FXN_TRACE;
    auto it = failedToSendToRemote.find(port);
    if (it == failedToSendToRemote.end()) {
        auto emplaceOk = failedToSendToRemote.emplace(port, spdlog::level::err);
        if (emplaceOk.second) {
            it = emplaceOk.first;
        }
    }
    if (it != failedToSendToRemote.end()) {
        it->second.log(fmt, std::forward<Args>(args)...);
    }
}

void UartCoapBridge::clearFailedToSendToRemote(uint16_t port) {
    FXN_TRACE;
    auto it = failedToSendToRemote.find(port);
    if (it != failedToSendToRemote.end()) {
        failedToSendToRemote.erase(it);
    }
}

void UartCoapBridge::sendRequest(const std::vector<uint8_t> &coapMsg, const uint16_t port) {
    FXN_TRACE;
    
    // Build message with room for tracking tokens
    size_t len = coapMsg.size();
    size_t maxLen = len + 3; // room for tracking tokens

    std::vector<uint8_t> buffer(maxLen);
    std::copy(coapMsg.begin(), coapMsg.end(), buffer.begin());

    coapPorts.udpToSerial(port, buffer.data(), &len, maxLen);

    spdlog::debug("Sending Observe request: MID {} -> port {}, msg = {}",
        Coap::getMid(buffer.data()), port,
        UartCoapBridge::dataToHex(buffer.data(), len));

    // Send to serial
    serialHandler->asyncSendMessageToSerialPort(
        buffer.data(),
        len);
}


void UartCoapBridge::sendObserveSRCPModeRequest(uint8_t observeValue) {
    FXN_TRACE;

    uint16_t port = 1001;
    std::vector<uint8_t> coapMsg = Coap::buildMessage(
        Coap::Type::CON,    // Confirmable message
        Coap::Method::GET,  // GET method
        0x1235,             // Message ID
        {"st", "mode"}, // URI Path segments
        {}, // No CBOR payload
        {}, // No Token (added later)
        Coap::Format::NONE, // No specific Content-Format
        true,   // Include Observe option
        observeValue   // Observe value
        );

    sendRequest(coapMsg, port);
}

void UartCoapBridge::sendObserveJoystickCombinedRequest(uint8_t observeValue) {
    FXN_TRACE;

    uint16_t port = 1000;
    std::vector<uint8_t> coapMsg = Coap::buildMessage(
        Coap::Type::CON,    // Confirmable message
        Coap::Method::GET,  // GET method
        0x1234,             // Message ID
        {"st", "joystick", "combined"}, // URI Path segments
        {}, // No CBOR payload
        {}, // No Token (added later)
        Coap::Format::NONE, // No specific Content-Format
        true,   // Include Observe option
        observeValue   // Observe value
        );

    sendRequest(coapMsg, port);
}