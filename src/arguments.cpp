#include "arguments.hpp"
//#include <ifaddrs.h>
//#include <net/if.h> // todo remove works without them
#include <arpa/inet.h>
#include <netdb.h>

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
                if (protocol == "udp" || protocol == "tcp") {
                    transport_protocol = protocol;
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
                printf_debug("Setting max retries to %s\n", argv[++i]);
                max_retries = std::stoi(argv[i]);
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

void arguments::resolve_address() {
    printf_debug("Resolving address\n");
    struct sockaddr_in sa;

    // Check if address is already a valid IPv4 address
    if (inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) == 1) {
        printf_debug("Address is already a valid IPv4 address\n");
        return;
    }

    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET; // Handles just IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(address.c_str(), nullptr, &hints, &res) != 0) {
        throw std::invalid_argument("Invalid target");
    }

    // Save the resolved address into the address string
    char resolved_address[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(((struct sockaddr_in*)res->ai_addr)->sin_addr), resolved_address, INET_ADDRSTRLEN) == nullptr) {
        std::cerr << "Error: Failed to convert resolved address to string.\n";
        freeaddrinfo(res);
        exit(1);
    }
    address = resolved_address;
    printf_debug("Resolved address: %s\n", address.c_str());
    freeaddrinfo(res);
}

void arguments::print_args() const {
    std::cout << "Transport: " << transport_protocol << std::endl;
    std::cout << "Address: " << address << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Timeout: " << timeout << std::endl;
    std::cout << "Max Retries: " << static_cast<int>(max_retries) << std::endl; // static cast so its visible in the output
}
