#ifndef FORT_AGENT_SPAMMYLOGMSG_H
#define FORT_AGENT_SPAMMYLOGMSG_H

#include <string>

#include <spdlog/spdlog.h>

#include <fort_agent/dbgTrace.h>

/** Simple class to handle log messages that might be emitted over and over and over due to a
    single event.
    Instead logs the first message, then only powers of 2 afterwards.
 */
class SpammyLogMsg {
    spdlog::level::level_enum logLevel;
    size_t count;
    size_t thresh;

public:
    SpammyLogMsg(spdlog::level::level_enum level) : logLevel(level), count(0),
                                                    thresh(1) {}

    template<typename FormatString, typename... Args>
    void log(const FormatString &fmt, Args &&...args) {
        count++;

        if (count >= thresh) {
            if (count == 1) {
                spdlog::log(logLevel, fmt, std::forward<Args>(args)...);
            } else {
                std::string newFmt = std::string(fmt) + " (x{})";
                spdlog::log(logLevel, newFmt, std::forward<Args>(args)...,
                            count);
            }

            thresh *= 2;
        }
    }

    void clear() {
        count = 0;
        thresh = 1;
    }
};


#endif //FORT_AGENT_SPAMMYLOGMSG_H
