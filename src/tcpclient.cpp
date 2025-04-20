#include "macro.hpp"

#include "tcpclient.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>

tcp_client::tcp_client(const std::string& address, int port) : display_name("unknown"){
    printf_debug("Creating TCP client\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf_debug("Socket created\n");
    if (sock == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // setting reuse address is not needed, operating system can choose any address to connect to the server but leaving it won't hurt i guess
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
    printf_debug("SO_REUSEADDR set on socket\n");

    printf_debug("Setting up address structure\n");
    address_struct.sin_family = AF_INET;
    address_struct.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &address_struct.sin_addr);
    printf_debug("Address structure set up\n");



    if (pipe(pipe_fds) == -1) {
        throw std::runtime_error("Failed to create pipe");
    }
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

void tcp_client::tcp_send(const std::string& message) {
    printf_debug("Sending message %s to server", message.c_str());
    if (send(sock, message.c_str(), message.size(), 0) == -1) {
        throw std::runtime_error("Failed to send message to server");
    }
    printf_debug("Message sent to server\n");
}

std::string tcp_client::tcp_receive() {
    printf_debug("Receiving message from server\n");
    char buffer[1024];
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        throw std::runtime_error("Failed to receive message from server");
    }

    // This buffer is for when the message is not ended with \r\n therefore it is segmented
    recv_buffer.append(buffer, bytes_received);
    if (!recv_buffer.ends_with("\r\n")) {
        return "";
    }
    std::string message = std::move(recv_buffer);
    recv_buffer.clear();
    return message;
}

std::string tcp_client::get_display_name() const {
    return display_name;
}

void tcp_client::set_display_name(const std::string& name) {
    display_name = name;
}

// Sends bye message on destruction
tcp_client::~tcp_client() {
    printf_debug("Sending Bye");
    std::ostringstream bye_stream;
    bye_stream << "BYE FROM " << get_display_name() << "\r\n";
    tcp_send(bye_stream.str());
    printf_debug("Bye sent");
    tcp_disconnect();
}
