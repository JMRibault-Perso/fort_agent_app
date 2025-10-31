#pragma once
#include <fort_agent/uartCoapBridge.h>

class UartCoapBridgeSingleton {
public:
    static UartCoapBridge& instance(
        boost::asio::io_service& service,
        const std::string& localAddr,
        uint16_t localPort,
        const std::string& serialPath,
        const std::string& remoteAddr,
        uint16_t remotePort
    );

    static UartCoapBridge& instance(); // safe overload

private:
    static std::unique_ptr<UartCoapBridge> bridgePtr;

    UartCoapBridgeSingleton() = delete;
    ~UartCoapBridgeSingleton() = delete;
};