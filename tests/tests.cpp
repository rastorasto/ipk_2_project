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

struct Server {
    Server(uint16_t port) : port(port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind to the socket");
        }

        if (listen(server_fd, 1) < 0) {
            throw std::runtime_error("Listening failed");
        }
    }

    void send_message(const std::string& message) {
        if (client_fd >= 0) {
            send(client_fd, message.c_str(), message.size(), 0);
        }
    }

    std::string receive_message() {
        char buffer[1024];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }

    void accept_client() {
        client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            throw std::runtime_error("Failed to accept client");
        }
    }

    ~Server() {
        if (client_fd >= 0) {
            close(client_fd);
        }
        if (server_fd >= 0) {
            close(server_fd);
        }
    }

    int server_fd;
    int client_fd;
    uint16_t port;
};

TEST_CASE("Authentication with OK Reply", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    // restore stdout
    std::cout.rdbuf(old_buf);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\n");
}

TEST_CASE("Authentication with NOK Reply", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY NOK IS Authentication failed\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    // restore stdout
    std::cout.rdbuf(old_buf);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Failure: Authentication failed\n");
}

TEST_CASE("Error in Start_State", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;

    server.send_message("ERR FROM test_server IS Meow a cat bit the cable and the server is down\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    REQUIRE(response == "ERR FROM test_server IS Meow a cat bit the cable and the server is down\r\n");
    state.process_response(fsm, response);

    // restore stdout
    std::cout.rdbuf(old_buf);

    std::string output = output_capture.str();
    REQUIRE(output == "ERROR FROM test_server: Meow a cat bit the cable and the server is down\n");
}

TEST_CASE("Bye in Start_State", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;

    server.send_message("BYE FROM test_server\r\n");

    std::string response = client.tcp_receive();
    REQUIRE(response == "BYE FROM test_server\r\n");
}

TEST_CASE("Authenticated Message send and receive", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_msg_message("test_username", "meeeow meow");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "MSG FROM test_username IS meeeow meow\r\n");

    server.send_message("MSG FROM dog IS haf haf\r\n");
    response = client.tcp_receive();
    open_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\ndog: haf haf\n");


    // restore stdout
    std::cout.rdbuf(old_buf);
}



TEST_CASE("Grammar insensitivity", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("RePlY oK iS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_msg_message("test_username", "meeeow meow");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "MSG FROM test_username IS meeeow meow\r\n");

    server.send_message("MsG FroM dog iS haf haf\r\n");
    response = client.tcp_receive();
    open_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\ndog: haf haf\n");


    // restore stdout
    std::cout.rdbuf(old_buf);
}

TEST_CASE("Received message in Auth_State", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("MSG FROM cat IS meow you should not see this message in auth state\r\n");

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    buffer = server.receive_message();
    REQUIRE(buffer == "ERR FROM unknown IS Message received in auth state.\r\n");

}

TEST_CASE("Received message from server missing \r\n", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("MSG FROM cat IS haha this message is not ended properly prrr");

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    buffer = server.receive_message();
    REQUIRE(buffer == "ERR FROM unknown IS Missing or malformed ERR message.\r\n");

}

TEST_CASE("Received message in 2 segments", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS ");
    server.send_message("Authentication successful\r\n");
    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_msg_message("test_username", "meeeow meow");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "MSG FROM test_username IS meeeow meow\r\n");

    server.send_message("MSG FROM dog ");
    server.send_message("IS haf haf\r\n");

    response = client.tcp_receive();
    response = client.tcp_receive();
    open_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\ndog: haf haf\n");

    // restore stdout
    std::cout.rdbuf(old_buf);
}

TEST_CASE("Authenticated Join", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("test_username", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH test_username AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_join_message("test_username", "channel_for_cats");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "JOIN channel_for_cats AS test_username\r\n");

    server.send_message("REPLY OK IS Hello fellow cat, welcome to our channel\r\n");
    response = client.tcp_receive();
    Join_State join_state;
    join_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\nAction Success: Hello fellow cat, welcome to our channel\n");

    // restore stdout
    std::cout.rdbuf(old_buf);
}

TEST_CASE("Segmented join reply", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("cat", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH cat AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_join_message("cat", "channel_for_cats");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "JOIN channel_for_cats AS cat\r\n");

    server.send_message("REPLY OK IS Hello fellow cat");
    server.send_message(", welcome to our channel\r\n");
    response = client.tcp_receive();
    response = client.tcp_receive();
    Join_State join_state;
    join_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\nAction Success: Hello fellow cat, welcome to our channel\n");

    // restore stdout
    std::cout.rdbuf(old_buf);
}

TEST_CASE("Authenticated Join Error", "[tcp_client]") {
    Server server(4567);

    tcp_client client("127.0.0.1", 4567);
    client.tcp_connect();

    FSM fsm(client);

    server.accept_client();

    Start_State state;
    std::string auth = state.create_auth_message("dog", "test_name", "test_secret");
    client.tcp_send(auth);

    std::string buffer = server.receive_message();

    REQUIRE(buffer == "AUTH dog AS test_name USING test_secret\r\n");

    // Since State is an unique pointer in fsm I can't use get function to access the fsm state therefore i simulate the change by creating a new state and using the process_response function on it. I am sure there is a better way to do this but this works too.
    Auth_State auth_state;

    server.send_message("REPLY OK IS Authentication successful\r\n");

    // redirect stdout
    std::stringstream output_capture;
    auto* old_buf = std::cout.rdbuf(output_capture.rdbuf());

    std::string response = client.tcp_receive();
    auth_state.process_response(fsm, response);

    Open_State open_state;
    std::string message = open_state.create_join_message("dog", "channel_for_cats");
    client.tcp_send(message);

    buffer = server.receive_message();
    REQUIRE(buffer == "JOIN channel_for_cats AS dog\r\n");

    server.send_message("ERR FROM SERVER IS Cat identifier returned error\r\n");
    response = client.tcp_receive();
    open_state.process_response(fsm, response);

    std::string output = output_capture.str();
    REQUIRE(output == "Action Success: Authentication successful\nERROR FROM SERVER: Cat identifier returned error\n");

    // restore stdout
    std::cout.rdbuf(old_buf);
}
