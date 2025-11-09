#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <fort_agent/dbgTrace.h>
#include <fort_agent/fort_agent.h>

namespace po = boost::program_options;

struct Configuration {
    std::string serial_port = "/dev/ttyACM0";
    int baud_rate = 115200;
    int coap_port = 5683;
    std::string log_level = "info";
    std::string log_file = "/var/log/fort-agent/fort-agent.log";
    std::string interface;
    std::string device;
    std::string remote_addr;
    std::string local_addr;
    uint16_t remote_port = 5683;
    uint16_t local_port = 0;
};

po::options_description getFortAgentOptions(Configuration& config) {
    po::options_description desc("FORT Agent Configuration");

    desc.add_options()
        ("help,h", "produce help message")
        ("version,V", "display fort_agent version")
        ("verbose,v", po::value<std::string>(&config.log_level)->implicit_value("trace"), "set verbosity")
        ("device,d", po::value<std::string>(&config.device)->required(), "serial device")
        ("remote,r", po::value<std::string>(&config.remote_addr)->required(), "Remote IP for safety traffic")
        ("port,p", po::value<uint16_t>(&config.local_port)->required(), "Local CoAP port")
        ("remote_port,q", po::value<uint16_t>(&config.remote_port)->implicit_value(5683), "Remote safety port")
        ("net,n", po::value<std::string>(&config.local_addr)->required(), "Local network interface")
        ("config,c", po::value<std::string>(), "Path to config file")
        ("log_file", po::value<std::string>(&config.log_file), "Log file path")
        ;

    return desc;
}

void setupDefaultLogger(const Configuration& config) {
    try {
        constexpr std::size_t max_file_size = 10 * 1024 * 1024; // 10 MB
        constexpr std::size_t max_files = 3; // Keep 3 rotated backups

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.log_file, max_file_size, max_files
        );

        auto logger = std::make_shared<spdlog::logger>("fort_agent", file_sink);
        spdlog::set_default_logger(logger);

        spdlog::level::level_enum level = spdlog::level::from_str(config.log_level);
        logger->set_level(level);
        logger->flush_on(spdlog::level::info);

        spdlog::info("Logger initialized with level '{}' and file '{}'", config.log_level, config.log_file);
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger setup failed: " << ex.what() << std::endl;
        std::exit(1);
    }
}



int main(int argc, char *argv[]) {
    FXN_TRACE;

    /*
        CoAP requests from NSC client -> Serial (SLIP decode) -> fort agent ephemeral UDP port -> EPC server
        CoAP responses from EPC server -> fort agent ephemeral UDP port -> Serial (SLIP encode) -> NSC client
        CoAP requests from EPC client -> fort agent bound UDP port -> CoAP tracker (insert tracking tokens) -> Serial (SLIP encode) -> NSC server
        CoAP responses from NSC server -> Serial (SLIP decode) -> CoAP tracker (extract tracking tokens) -> fort agent bound UDP port -> EPC client
     */

    Configuration config;
    po::options_description desc = getFortAgentOptions(config);
    po::variables_map vm;

    try {
        // First: parse CLI
        po::store(po::parse_command_line(argc, argv, desc), vm);

        // Check for config file
        if (vm.count("config")) {
            std::ifstream cfg(vm["config"].as<std::string>());
            if (!cfg) throw std::runtime_error("Cannot open config file");
            po::store(po::parse_config_file(cfg, desc), vm);
        }

        // Apply all options
        po::notify(vm);

        // Now that config is populated, set up logging
        setupDefaultLogger(config);

        // Handle help/version
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("version")) {
            std::cout << "Fort Agent Version: " << getFortAgentVersion() << std::endl;
            return 0;
        }

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse options\n" << desc << "\n" << e.what() << std::endl;
        return 1;
    }

    spdlog::info("FORT Agent Version {}", getFortAgentVersion());
    spdlog::info("Using serial port = {}", config.device);
    spdlog::info("Listening for CoAP traffic from {}:{}", config.remote_addr, config.remote_port);
    spdlog::info("Hosting CoAP server on {}:{}", config.local_addr, config.local_port);
    spdlog::info("FORT Agent starting up");

    boost::asio::io_service ioService;

    auto flushTimer = std::make_shared<boost::asio::steady_timer>(ioService);

    std::function<void()> scheduleFlush = [&]() {
        flushTimer->expires_after(std::chrono::milliseconds(100));
        flushTimer->async_wait([&](const boost::system::error_code& ec) {
            if (!ec) {
                printToConsole.flushConsoleMessages();
                scheduleFlush(); // reschedule
            }
        });
    };

    scheduleFlush(); // start the loop

    try {
        // Initialize JAUS Bridge
        // Create JAUS client implementation
        auto client = std::make_unique<JAUSClientImpl>();
        
        //TODO: set JAUS configuration using system::Application::setOjConfiguration(JSON);
        // This configuration should be read from the .ojconf file and passed to OpenJAUS

        // Get singleton instance of JausBridge
        auto& jausBridge = JausBridgeSingleton::instance(std::move(client));

        // Initialize UART-CoAP Bridge
        auto& uartCoapBridge = UartCoapBridgeSingleton::instance( 
            ioService, config.local_addr, config.local_port,
            config.device, config.remote_addr, config.remote_port);

        // Start JAUS service loop, must be done after UartCoapBridge is initialized
        jausBridge.startServiceLoop(); 
       
        // Start IO service loop
        ioService.run();
        return 0;
    }
    catch (std::runtime_error &e) {
        spdlog::critical("Stopping fort_agent due to fatal exception: {}",
                        e.what());
        ioService.stop();
        spdlog::drop_all();
        return 2;
    }
    catch (std::exception &e) {
        spdlog::critical("Stopping fort_agent due to fatal exception: {}",
                        e.what());
        ioService.stop();
        spdlog::drop_all();
        return 3;
    }
    catch (...) {
        spdlog::critical(
            "Stopping fort_agent due to fatal but unknown exception");
        ioService.stop();
        spdlog::drop_all();
        return 4;
    }


}
