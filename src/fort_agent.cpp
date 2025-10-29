#include <iostream>
#include <fort_agent/fort_agent.h>

std::string getFortAgentVersion() {
    return std::string(fort_agent_VERSION);
}

ConsoleBuffer printToConsole;

void ConsoleBuffer::flush() {
    std::string msg = stream_.str();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(msg);
    }
    stream_.str("");
    stream_.clear();
}

void ConsoleBuffer::flushString(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(msg);
}

void ConsoleBuffer::flushConsoleMessages() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        std::cout << queue_.front();
        queue_.pop();
    }
}
