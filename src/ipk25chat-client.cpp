#define DEBUG_PRINT // todo remove before submission
#include "macro.hpp"

#include "ipk25chat-client.hpp"
#include "arguments.hpp"
#include "tcpclient.hpp"
#include "fsm.hpp"
#include <iostream>

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

    FSM fsm; // todo fsm will work with client
    std::string input;
    while (std::getline(std::cin, input)) {
        fsm.process_client_input(input);
    }

    printf_debug("Programm ending");
    return 0;
}
