#include "arguments.hpp"

#define DEBUG_PRINT
#include "macro.hpp"

void arguments::help() const {
    std::cout << "Usage: ipk25chat-client [options]\n";
    std::cout << "-t 	User provided 	tcp or udp 	Transport protocol used for connection\n";
    std::cout << "-s 	User provided 	IP address or hostname 	Server IP or hostname\n";
    std::cout << "-p 	4567      uint16 	Server port\n";
    std::cout << "-d 	250       uint16 	UDP confirmation timeout (in milliseconds)\n";
    std::cout << "-r 	3         uint8  	Maximum number of UDP retransmissions\n";
    std::cout << "-h 	Prints program help output and exits\n";
}

arguments::arguments(int argc, char* argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        printf_debug("Processing argument: %s", argv[i]);
        std::string arg = argv[i];
        if (arg == "-t") {
            if (i + 1 < argc) {
                std::string protocol = argv[++i];
                if (protocol == "udp") {
                    transport_protocol = false;
                } else if (protocol == "tcp") {
                    transport_protocol = true;
                } else {
                    std::cerr << "Error: Invalid transport protocol. Use -h for help.\n";
                    exit(1);
                }
            } else {
                std::cerr << "Error: Missing value for -t option. Use -h for help.\n";
                exit(1);
                }
        } else if (arg == "-s") {
            if (i + 1 < argc) {
                address = argv[++i];
            }
        } else if (arg == "-p") {
            if (i + 1 < argc) {
                port = std::stoi(argv[++i]);
            }
        } else if (arg == "-d") {
            if (i + 1 < argc) {
                timeout = std::stoi(argv[++i]);
            }
        } else if (arg == "-r") {
            if (i + 1 < argc) {
                max_retries = std::stoi(argv[++i]);
            }
        } else if (arg == "-h") {
            help();
            exit(0);
        } else {
            std::cerr << "Error: Invalid argument. Use -h for help.\n";
            exit(1);
        }
    }
}

void arguments::print_args() const {
    printf_debug("Transport: %s", transport_protocol ? "tcp" : "udp");
    printf_debug("Address: %s", address.c_str());
    printf_debug("Port: %u", port);
    printf_debug("Timeout: %u", timeout);
    printf_debug("Max Retries: %u", max_retries);
}
