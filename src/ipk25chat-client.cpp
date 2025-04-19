#include "macro.hpp"

#include "ipk25chat-client.hpp"
#include "arguments.hpp"
#include "tcpclient.hpp"
#include "fsm.hpp"
#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>

#include <csignal>

int main(int argc, char* argv[]) {
    try{

    printf_debug("Program started!");
    arguments args(argc, argv);

    args.resolve_address();

    tcp_client client(args.address, args.port);
    client.tcp_connect();


    FSM fsm(client);

    fd_set read_fds;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(client.sock, &read_fds);

        FD_SET(client.pipe_fds[0], &read_fds);
        int max_fd = std::max({STDIN_FILENO, client.sock, client.pipe_fds[0]});
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            printf_debug("uh oh");
            std::cerr << "select error" << std::endl;
            break;
        }

        // If there is data to read break the loop
        if (FD_ISSET(fsm.client.pipe_fds[0], &read_fds)) {
            printf_debug("Reading client pipe");
            char buf;
            read(client.pipe_fds[0], &buf, 1);
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
                fsm.handle_bye();
                break;
            }
        }

        if (FD_ISSET(client.sock, &read_fds)) {
            printf_debug("Reading server response");
            std::string response = client.tcp_receive();
            fsm.process_server_response(response);
        }
    }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    printf_debug("Programm ending");
    return 0;
}
