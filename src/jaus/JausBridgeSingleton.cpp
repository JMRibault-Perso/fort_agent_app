#include <stdexcept>
#include <fort_agent/jaus/JausBridgeSingleton.h>

std::unique_ptr<JausBridge> JausBridgeSingleton::bridgePtr = nullptr;

JausBridge& JausBridgeSingleton::instance(std::unique_ptr<JAUSClient> client) {
    if (!bridgePtr) {
        bridgePtr = std::make_unique<JausBridge>(std::move(client));
    }
    return *bridgePtr;
}

JausBridge& JausBridgeSingleton::instance() {
    if (!bridgePtr) {
        throw std::runtime_error("JausBridge not initialized");
    }
    return *bridgePtr;
}