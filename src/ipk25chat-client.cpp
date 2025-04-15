#define DEBUG_PRINT // todo remove before submission
#include "macro.hpp"

#include "ipk25chat-client.hpp"
#include "arguments.hpp"
#include "tcpclient.hpp"
#include "fsm.hpp"
#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    printf_debug("Program started!");

    arguments args(argc, argv);
    if(args.transport_protocol.empty() || args.address.empty()) {
        std::cerr << "Error: Missing arguments. Use -h for help.\n" << std::endl;
        return 1;
    }

    args.resolve_address();
    args.print_args();

    tcp_client client(args.address, args.port);
    client.tcp_connect();

    FSM fsm(client);

    // std::string input;
    // while (std::getline(std::cin, input)) { // change into file descriptors so when server replies it doesnt wait for user input
    //     fsm.process_client_input(input);
    // }

    fd_set read_fds;
    int max_fd = std::max(STDIN_FILENO, client.sock);

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client.sock, &read_fds);

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            printf_debug("uh oh");
            std::cerr << "select error" << std::endl;
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            printf_debug("Reading user input");
            std::string input;
            if (std::getline(std::cin, input)) {
                printf_debug("Processing user input");
                fsm.process_client_input(input);
            } else {
                std::cout << "End of user input." << std::endl;
                break;
            }
        }

        if (FD_ISSET(client.sock, &read_fds)) {
            printf_debug("Reading server response");
            std::string response = client.tcp_receive();
            fsm.process_server_response(response);
        }
    }

    printf_debug("Programm ending");
    return 0;
}
