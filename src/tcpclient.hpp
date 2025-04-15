#pragma once

#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

struct tcp_client {
    tcp_client(const std::string& address, int port);
    ~tcp_client();

    void tcp_connect();
    void tcp_disconnect();
    void tcp_send(const std::string& message);
    std::string tcp_receive();
    void set_display_name(const std::string& display_name);
    std::string get_display_name() const;
    int sock;

    private:
    struct sockaddr_in address_struct;
    std::string display_name;
};
