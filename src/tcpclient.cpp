#define DEBUG_PRINT // todo remove before submission
#include "macro.hpp"

#include "tcpclient.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


tcp_client::tcp_client(const std::string& address, int port) {
    printf_debug("Creating TCP client\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf_debug("Socket created\n");
    if (sock == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    printf_debug("Setting up address structure\n");
    address_struct.sin_family = AF_INET;
    address_struct.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &address_struct.sin_addr);
    printf_debug("Address structure set up\n");
}

void tcp_client::tcp_connect() {
    printf_debug("Connecting to server\n");
    if (connect(sock, (struct sockaddr*)&address_struct, sizeof(address_struct)) == -1) {
        throw std::runtime_error("Failed to connect to server");
    }
    printf_debug("Connected to server\n");
}

void tcp_client::tcp_disconnect() {
    printf_debug("Disconnecting from server\n");
    if (close(sock) == -1) {
        throw std::runtime_error("Failed to disconnect from server");
    }
    printf_debug("Disconnected from server\n");
}

tcp_client::~tcp_client() {
    tcp_disconnect();
}
