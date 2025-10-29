#ifndef FORT_AGENT_UARTCOAPBRIDGE_H
#define FORT_AGENT_UARTCOAPBRIDGE_H

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

#include <fort_agent/dbgTrace.h>
#include <fort_agent/coapPortTracker.h>
#include <fort_agent/serialHandler.h>
#include <fort_agent/spammyLogMsg.h>

class UartCoapBridge {
public:
    UartCoapBridge(
        boost::asio::io_service &service,
        const std::string &localAddr,
        uint16_t localPort,
        const std::string &serialPath,
        const std::string &remoteAddr,
        uint16_t remotePort,
        const std::string &jausRemoteAddr,
        uint16_t jausRemotePort
    );

    ~UartCoapBridge() = default;

private:
    // Reference to the boost asio service
    boost::asio::io_service &ioService;

    // Handle the serial interface
    std::unique_ptr<SerialHandler> serialHandler;

    void serialError(const std::string &errMsg);

    void serialDataReceived(uint8_t *message, uint32_t size);

    // Listen on local UDP port
    uint16_t listenPort;
    boost::asio::ip::address localHost;
    boost::asio::ip::udp::endpoint localEndpoint;
    boost::asio::ip::udp::socket localSocket;
    boost::asio::deadline_timer localBindRetryTimer;
    boost::asio::ip::udp::endpoint from;  // Dummy variable used by async_recv_from, when receiving UDP from local clients
    static constexpr int localBindRetrySeconds = 5;
    boost::asio::ip::address remoteHost;
    boost::asio::ip::address jausHemoteHost;
    uint16_t jausRemotePort;

    void bindLocal();

    void receiveFromRemote();

    void sendToRemote(boost::asio::ip::udp::endpoint to,
                      std::shared_ptr<uint8_t[]> data, std::size_t length);

    void sendObserveJoystickCombinedRequest(uint8_t observeValue = 0);
    void sendObserveSRCPModeRequest(uint8_t observeValue = 0);
    void sendRequest(const std::vector<uint8_t> &coapMsg, const uint16_t port);
    // Relay received CoAP messages to the right client
    CoapPortTracker coapPorts;

    // Reduce repetitive log spam
    SpammyLogMsg failedToBindLocal;
    SpammyLogMsg failedToReceiveFromRemote;
    SpammyLogMsg failedToSendSerial;
    SpammyLogMsg failedToListenSerial;

    std::map<uint16_t, SpammyLogMsg> failedToSendToRemote;

    template<typename FormatString, typename... Args>
    void logFailedToSendToRemote(uint16_t port, const FormatString &fmt,
                                 Args &&...args);

    void clearFailedToSendToRemote(uint16_t port);

        // Debug helper
    static std::string dataToHex(const void *data, size_t len);

};

#endif //FORT_AGENT_UARTCOAPBRIDGE_H
