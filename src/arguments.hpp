#pragma once

#include <iostream>

struct arguments {
    bool transport_protocol; // 0 is UDP, 1 is TCP
    std::string address;
    uint16_t port = 4567;
    uint16_t timeout = 250; // in milliseconds
    uint8_t max_retries = 3; // only in udp

    void help() const;
    arguments(int argc, char* argv[]);
    void print_args() const;
};
