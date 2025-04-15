#pragma once

#include <iostream>

struct arguments {
    std::string transport_protocol;
    std::string address;
    uint16_t port = 4567;
    uint16_t timeout = 250; // in milliseconds
    uint8_t max_retries = 3; // only in udp

    void help() const;
    arguments(int argc, char* argv[]);
    void resolve_address();
    void print_args() const;
};
