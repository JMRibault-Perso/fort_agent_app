#ifndef FORT_AGENT_DBG_TRACE_H
#define FORT_AGENT_DBG_TRACE_H

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#ifdef ENABLE_FXN_TRACE
#define __FILENAME__ std::max<const char*>(__FILE__,\
    std::max(strrchr(__FILE__, '\\')+1, strrchr(__FILE__, '/')+1))
#define FXN_TRACE FunctionTrace(__FILENAME__, __FUNCTION__, __LINE__)
#define WATCH(var) spdlog::trace("{} = {}", #var, str::string(var))
#else
#define FXN_TRACE
#define WATCH(var)
#endif

#define ENABLE_STAT_TRACE
#ifdef ENABLE_STAT_TRACE
#define COUNT_REQUEST_TO_EPC StatTrace::incrementRequestsToEpc()
#define COUNT_REQUEST_TO_NSC StatTrace::incrementRequestsToNsc()
#define COUNT_RESPONSE_FROM_EPC StatTrace::incrementResponsesFromEpc()
#define COUNT_RESPONSE_FROM_NSC StatTrace::incrementResponsesFromNsc()
#else
#define COUNT_REQUEST_TO_EPC
#define COUNT_REQUEST_TO_NSC
#define COUNT_RESPONSE_FROM_EPC
#define COUNT_RESPONSE_FROM_NSC
#endif

class FunctionTrace {
    std::string _file;
    std::string _function;
    int _line;
public:
    FunctionTrace(std::string file, std::string function, int line) : _file(
        file), _function(function), _line(line) {
        spdlog::trace("--- Entering {} in [{}::{}]", _function, _file, _line);
    }

    ~FunctionTrace() {
        spdlog::trace("--- Leaving {} in [{}]", _function, _file);
    }
};

class StatTrace {
    inline static int RequestsToEpc;
    inline static int RequestsToNsc;

    inline static int ResponsesFromEpc;
    inline static int ResponsesFromNsc;

public:
    StatTrace(boost::asio::io_service &service) :
        statTraceTimer(service) {
        resetStats();
        printStats();
    };

    boost::asio::deadline_timer statTraceTimer;
    static constexpr int statTracePeriodSeconds = 10;

    static void DisplayAllData() {
        spdlog::info("Requests To Epc    : {}", RequestsToEpc);
        spdlog::info("Requests To Nsc    : {}", RequestsToNsc);
        spdlog::info("Responses From Epc : {}", ResponsesFromEpc);
        spdlog::info("Responses From Nsc : {}", ResponsesFromNsc);
    }

    static void resetStats() {
        RequestsToEpc = 0;
        RequestsToNsc = 0;
        ResponsesFromEpc = 0;
        ResponsesFromNsc = 0;
    }


    static void incrementRequestsToEpc() { RequestsToEpc++; };

    static void incrementRequestsToNsc() { RequestsToNsc++; };

    static void incrementResponsesFromEpc() { ResponsesFromEpc++; };

    static void incrementResponsesFromNsc() { ResponsesFromNsc++; };

    void printStats() {
        DisplayAllData();

        statTraceTimer.expires_from_now(
            boost::posix_time::seconds(statTracePeriodSeconds));
        statTraceTimer.async_wait(
            [this](const boost::system::error_code & /*e*/) {
                printStats();
            });
    }
};

#endif //FORT_AGENT_DBG_TRACE_H
