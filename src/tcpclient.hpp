#pragma once

#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

struct tcp_client {
    tcp_client(const std::string& address, int port);
    ~tcp_client();

    void tcp_connect();
    void tcp_disconnect();
    void send(const std::string& message);
    std::string receive();

private:
    int sock;
    struct sockaddr_in address_struct;
};
