#include <fort_agent/uartCoapBridgeSingleton.h>

std::unique_ptr<UartCoapBridge> UartCoapBridgeSingleton::bridgePtr = nullptr;

UartCoapBridge& UartCoapBridgeSingleton::instance(
    boost::asio::io_service& service,
    const std::string& localAddr,
    uint16_t localPort,
    const std::string& serialPath,
    const std::string& remoteAddr,
    uint16_t remotePort
) {
    if (!bridgePtr) {
        bridgePtr = std::make_unique<UartCoapBridge>(
            service, localAddr, localPort, serialPath, remoteAddr, remotePort
        );
    }
    return *bridgePtr;
}

UartCoapBridge& UartCoapBridgeSingleton::instance() {
    if (!bridgePtr) {
        throw std::runtime_error("UartCoapBridge not initialized");
    }
    return *bridgePtr;
}