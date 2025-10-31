// JausBridgeSingleton.h
#pragma once
#include <fort_agent/jaus/JausBridge.h>
#include <fort_agent/jaus/JausClient.h>

class JausBridgeSingleton {
public:
    static JausBridge& instance(std::unique_ptr<JAUSClient> client);
    static JausBridge& instance();

private:
    static std::unique_ptr<JausBridge> bridgePtr;

    JausBridgeSingleton() = delete;
    ~JausBridgeSingleton() = delete;
};


