# FORT Agent

## About The Project
### What
The purpose of this project is to built a software agent capable of transmitting
safety data to/from the NSC over an IP network. At the time of writing only 3
methods of passing safety traffic on the NSC exists:
1. BLE Radio
2. ISM Radio
3. Serial (UART or USB)

### Why 
This was done at the behest of a customer, the original notes about this project
cant be found here :
https://hriwiki.atlassian.net/wiki/spaces/HRIIN/pages/2315813055/Agility

## Getting Started

### Prerequisites
1. C++ 17 Compiler
2. cmake (v3.11 or greater)

### Dependencies
At the moment these are pulled in by cmake `FetchContent`
1. googletest: `release-1.11.0` -- https://github.com/google/googletest.git
2. boost: `boost-1.78.0` -- https://github.com/boostorg/boost.git
3. fmtlib: `8.1.1` -- https://github.com/fmtlib/fmt.git
4. spdlog: `v1.9.2` -- https://github.com/gabime/spdlog.git

### Compilation

This is a cmake project and can be built as follows:
```
# Clone the repo.
git clone git@bitbucket.org:hriinc/fort-agent.git

# Go to the main directory of the project.
cd fort-agent/fort-agent-cpp

# Create a directory to hold the build output.
mkdir build

# Generate a build system
cmake -S . -B build

# Build project binary tree
cmake --build build --target fort_agent_app
```

#### Targets
1. `fort_agent_app` -- Main application
2. `fort_agent_lib` -- Dont need typically
3. `fort_agent_test` -- Dont need typically

### Instalation
At the moment this is a statically compiled standalone application.


## Usage
```
> ./apps/fort_agent_app --help
fort_agent_app is an IP-UART bridge for the FORT NSC:
  -h [ --help ]                      produce help message
  -V [ --version ]                   display fort_agent version
  -v [ --verbose ] [=arg(=trace)]    set verbosity: trace, debug, info, warning
                                     or warn, error or err, critical, off
  -d [ --device ] arg                serial device (e.g. /dev/ttyACM_ or
                                     /dev/ttyUSB_)
  -r [ --remote ] arg                Remote IP to pull safety traffic
  -p [ --port ] arg                  Local Port to listen for CoAP requests
  -q [ --remote_port ] [=arg(=5683)] Remote port to send CoAP requests
  -n [ --net ] arg                   Local Network interface to handle
                                     traffic.This is is the IP of the specific
                                     network adapter to bind or 0.0.0.0 to
                                     respond through any interface
```

### Example
```
fort_agent_app --device /dev/ttyUSB0 --net 0.0.0.0 --port 12000 --remote 127.0.0.1 --remote_port 5683
```

The `--net` and --port` options go together, and the `--remote` and `--remote_port` options go together. That is to say, as per the example above, the switch `--net 0.0.0.0 --port 12000` tells `fort_agent_app` to RECEIVE data from `0.0.0.0:12000` and a switch of `--remote 127.0.0.1 --remote_port 5683` tells `fort_agent_app` to SEND CoAP data to `127.0.0.1:5683`.

To view CoAP traffic in Wireshark, you need to use the standard port of either 5683 or 5684 - Wireshark is not able to dynamically recognize CoAP packets from traffoc on any other ports.

## Internal

### Compile debug options
1. `ENABLE_FXN_TRACE` -- Trace Execution of Program
2. `ENABLE_STAT_TRACE` -- Get Transfer statistics from fort_agent

### Notes
This application handles traffic in 4 ways: 

1. CoAP requests from NSC client -> Serial (SLIP decode) -> fort agent ephemeral UDP port -> EPC server
2. CoAP responses from EPC server -> fort agent ephemeral UDP port -> Serial (SLIP encode) -> NSC client
3. CoAP requests from EPC client -> fort agent bound UDP port -> CoAP tracker (insert tracking tokens) -> Serial (SLIP encode) -> NSC server
4. CoAP responses from NSC server -> Serial (SLIP decode) -> CoAP tracker (extract tracking tokens) -> fort agent bound UDP port -> EPC client


## License
FORT Robotics Proprietary

## Contact
Max Hirsch - max.hirsch@fortrobotics.com

Project Link: https://bitbucket.org/hriinc/fort-agent

## Changelog
### [1.0.0] - Initial release
### [1.0.1] - Sept 19, 2022
- Add option to specify both remote and local port manually (as opposed to being locked to 5683 and an arbitrary one)
