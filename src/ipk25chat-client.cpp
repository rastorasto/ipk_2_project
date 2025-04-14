#define DEBUG_PRINT // todo remove before submission
#include "macro.hpp"

#include "ipk25chat-client.hpp"
#include "arguments.hpp"



int main(int argc, char* argv[]) {
    printf_debug("Program started!");
    arguments args(argc, argv);

    args.print_args();

    return 0;
}
