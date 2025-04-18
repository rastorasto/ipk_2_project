#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <memory>

#include "arguments.hpp"
#include "fsm.hpp"

TEST_CASE("Default arguments values", "[cli]") {
    // Added the required arguments because otherwise the constructor throws and exception (this is tested in test Required arguments not provided)
    const char* argv[] = { "ipk25chat_client", "-t", "tcp", "-s", "127.0.0.1"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    arguments args(argc, const_cast<char**>(argv));

    REQUIRE(args.transport_protocol == "tcp");
    REQUIRE(args.address == "127.0.0.1");
    REQUIRE(args.port == 4567);
    REQUIRE(args.timeout == 250);
    REQUIRE(args.max_retries == 3);
}

TEST_CASE("Custom arguments TCP", "[cli]") {
    const char* argv[] = { "ipk25chat_client", "-t", "tcp", "-s", "127.0.0.1", "-p", "1234", "-d", "500", "-r", "5" }; // todo show some error when chosen protocol is tcp and user provides confimation timeout and retrasmission which are udp only.
    // but this is just validating that arguments work
    int argc = sizeof(argv) / sizeof(argv[0]);

    arguments args(argc, const_cast<char**>(argv));

    REQUIRE(args.transport_protocol == "tcp");
    REQUIRE(args.address == "127.0.0.1");
    REQUIRE(args.port == 1234);
    REQUIRE(args.timeout == 500);
    REQUIRE(args.max_retries == 5);
}

TEST_CASE("Custom arguments UDP", "[cli]") {
    const char* argv[] = { "ipk25chat_client", "-t", "udp", "-s", "127.0.0.1", "-p", "1234", "-d", "500", "-r", "5" };
    int argc = sizeof(argv) / sizeof(argv[0]);

    arguments args(argc, const_cast<char**>(argv));

    REQUIRE(args.transport_protocol == "udp");
    REQUIRE(args.address == "127.0.0.1");
    REQUIRE(args.port == 1234);
    REQUIRE(args.timeout == 500);
    REQUIRE(args.max_retries == 5);
}

TEST_CASE("Required arguments not provided", "[cli]") {
    const char* argv[] = { "ipk25chat_client" };
    int argc = 1;

    REQUIRE_THROWS_WITH(arguments(argc, const_cast<char**>(argv)), "Missing arguments. Use -h for help.");
}

struct MockState : State {
  void process_input(FSM&, const std::string&)    override {}
  void process_response(FSM&, const std::string&) override {}
  std::string name() const override { return "Mock"; }
};

TEST_CASE("Create message", "[fsm]") {
    MockState state;
    std::string display_name = "test_name";
    std::string message_content = "This is a test message";

    std::string token;
    token = state.create_msg_message(display_name, message_content);

    REQUIRE(token == "MSG FROM test_name IS This is a test message\r\n");
}

TEST_CASE("Message is shortened if its longer than max length", "[fsm]") {
    MockState state;
    std::string display_name = "test_name";
    std::string message_content(70000, 'a'); // Max length exceeded
    std::string message_content_cut = message_content.substr(0, 60000);

    std::string token;
    token = state.create_msg_message(display_name, message_content);

    REQUIRE(token == "MSG FROM test_name IS " + message_content_cut + "\r\n");

}

TEST_CASE("Create auth message", "[fsm]") {
    MockState state;
    std::string username = "test_username";
    std::string display_name = "test_name";
    std::string secret = "test_secret";

    std::string token;
    token = state.create_auth_message(username, display_name, secret);

    REQUIRE(token == "AUTH test_username AS test_name USING test_secret\r\n");
}

TEST_CASE("Create join message", "[fsm]") {
    MockState state;
    std::string display_name = "test_name";
    std::string channel_name = "test_channel";
    std::string token;
    token = state.create_join_message(display_name, channel_name);

    REQUIRE(token == "JOIN test_channel AS test_name\r\n");
}
