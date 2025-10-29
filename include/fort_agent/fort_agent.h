#pragma once

#include <sstream>
#include <mutex>
#include <queue>
#include <string>

#include <fort_agent/dbgTrace.h>
#include <fort_agent/uartCoapBridge.h>

std::string getFortAgentVersion();

// Stream-like console output
class ConsoleBuffer {
public:
    template<typename T>
    ConsoleBuffer& operator<<(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        stream_ << value;
        return *this;
    }

    ConsoleBuffer& operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(mutex_);
        stream_ << manip;

        if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl) ||
            manip == static_cast<std::ostream& (*)(std::ostream&)>(std::flush)) {
            flush();
        }

        return *this;
    }

    void flushString(const std::string& msg);
    // Flush queue from main thread
    void flushConsoleMessages();

private:
    void flush();

    std::ostringstream stream_;
    std::mutex mutex_;
    std::queue<std::string> queue_;
};

// Global instance
extern ConsoleBuffer printToConsole;




