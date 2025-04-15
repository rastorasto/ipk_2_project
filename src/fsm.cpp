#include "fsm.hpp"
#include <sstream>
#include <iostream>

#define DEBUG_PRINT
#include "macro.hpp"
#include "tcpclient.hpp"

FSM::FSM(tcp_client& client) : client(client), state(std::make_unique<Start_State>()){}

void FSM::process_client_input(const std::string& input) {
    state->process_input(*this, input);
}

void FSM::process_server_response(const std::string& response) {
    state->process_response(*this, response);
}

void FSM::change_state(std::unique_ptr<State> new_state) {
    printf_debug("Changing state from %s to %s", state->name().c_str(), new_state->name().c_str());
    state = std::move(new_state);
}

// ------------ Start_State ------------

void Start_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Start State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command %s received", command.c_str());

    if (command == "/auth") {
        std::string username, secret, display_name;
        iss >> username >> secret >> display_name;
        fsm.client.set_display_name(display_name);

        printf_debug("Username: %s", username.c_str());
        printf_debug("Secret: %s", secret.c_str());
        printf_debug("Display Name: %s", display_name.c_str());

        std::ostringstream token_stream;
        token_stream << "AUTH " << username << " AS " << fsm.client.get_display_name() << " USING " << secret << "\r\n";
        std::string token = token_stream.str();
        printf_debug("Token: %s", token.c_str());
        fsm.client.tcp_send(token);
        printf_debug("Token sent to server");

        fsm.change_state(std::make_unique<Auth_State>());
    } else if (command == "/bye") {
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

void Start_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Start State");
    printf_debug("Response: %s", response.c_str());
    fsm.change_state(std::make_unique<End_State>());
}

std::string Start_State::name() const {
    return "Start_State";
}

// ------------ Auth_State ------------

void Auth_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Auth State");

    fsm.change_state(std::make_unique<Open_State>());

    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command %s received", command.c_str());

    if (command == "/auth") {
        fsm.change_state(std::make_unique<Auth_State>());
    } else if (command == "/bye") {
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

void Auth_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Auth State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    std::string value;
    std::string is;
    std::string message_content;
    iss >> fsm_name >> value >> is >> message_content;

    if(fsm_name == "REPLY" && value == "NOK"){
        printf_debug("Authentication failed %s", message_content.c_str());
        fsm.change_state(std::make_unique<Auth_State>());
    } else if(fsm_name == "REPLY" && value == "OK"){
        printf_debug("Authentication successful %s", message_content.c_str());
        fsm.change_state(std::make_unique<Open_State>());
    } else { // todo implement err,bye ..
        printf_debug("Invalid response %s", message_content.c_str());
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Auth_State::name() const {
    return "Auth_State";
}

// ----------- Open_State ------------

void Open_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Open State");

    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command: %s", command.c_str());

    if(command == "/msg"){ // MSG
        std::string message_content;
        std::getline(iss, message_content);

        std::ostringstream token_stream;
        token_stream << "MSG FROM " << fsm.client.get_display_name() << " IS " << message_content << "\r\n";
        std::string token = token_stream.str();
        printf_debug("Sending Token %s to Server", token.c_str());
        fsm.client.tcp_send(token);

    } else if (command == "/join") { // JOIN
        std::string channel_name;
        iss >> channel_name;

        std::ostringstream token_stream;
        token_stream << "JOIN " << channel_name << " AS " << fsm.client.get_display_name() << "\r\n";
        std::string token = token_stream.str();
        printf_debug("Sending Token %s to Server", token.c_str());
        fsm.client.tcp_send(token);

        fsm.change_state(std::make_unique<Join_State>());
    } else if (command == "/bye") {
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

void Open_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Open State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;
    if(fsm_name == "MSG"){
        std::string from_keyword;
        iss >> from_keyword;
        if(from_keyword != "FROM"){
            printf_debug("Invalid grammar"); // todo should not happen but just to be sure
        }
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        if(is_keyword != "IS"){
            printf_debug("Invalid grammar"); // todo should not happen but just to be sure
        }
        std::string message_content;
        std::getline(iss, message_content);
        std::cout << name << ": " << message_content << std::endl;
    } else if(fsm_name == "REPLY"){
        std::string status;
        iss >> status;
        if(status == "OK"){
            std::string is_keyword;
            iss >> is_keyword;
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // todo should not happen but just to be sure
            }
            std::string message_content;
            std::getline(iss, message_content);
            std::cout << "Action Success:" << message_content << std::endl;
        } else if(status == "ERROR"){
            std::string is_keyword;
            iss >> is_keyword;
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // todo should not happen but just to be sure
            }
            std::string message_content;
            std::getline(iss, message_content);
            // Action Failure: {MessageContent}\n
            std::cout << "Action Failure:" << message_content << std::endl;
        } else {
            std::cout << "ERROR: Invalid status " << status << std::endl;
            printf_debug("Invalid status in open state ");
        }
    } else { // todo implement err, bye ...
        std::cout << "ERROR: Invalid fsmname" << fsm_name << std::endl;
        printf_debug("Invalid fsmname in open state ");
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Open_State::name() const {
    return "Open_State";
}

// ----------- Join_State ------------

void Join_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Join State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    printf_debug("Command: %s", command.c_str());

    if (command == "/bye") {
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

void Join_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Join State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;

    if(fsm_name == "MSG"){
        std::string from_keyword;
        iss >> from_keyword;
        if(from_keyword != "FROM"){
            printf_debug("Invalid grammar");
        }
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        if(is_keyword != "IS"){
            printf_debug("Invalid grammar");
        }
        std::string message_content;
        std::getline(iss, message_content);
        std::cout << name << ": " << message_content << std::endl;
    } else if(fsm_name == "REPLY"){
        std::string status;
        iss >> status;
        if(status == "OK"){
            std::string is_keyword;
            iss >> is_keyword;
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // todo should not happen but just to be sure
            }
            std::string message_content;
            std::getline(iss, message_content);
            std::cout << "Action Success:" << message_content << std::endl;
            fsm.change_state(std::make_unique<Open_State>());
        } else if(status == "ERROR"){
            std::string is_keyword;
            iss >> is_keyword;
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // todo should not happen but just to be sure
            }
            std::string message_content;
            std::getline(iss, message_content);
            // Action Failure: {MessageContent}\n
            std::cout << "Action Failure:" << message_content << std::endl;
            fsm.change_state(std::make_unique<Open_State>());
        } else {
            std::cout << "ERROR: Invalid status " << status << std::endl;
            printf_debug("Invalid status in open state ");
        }
    } else { // todo implement msg/_ err,bye/_ _/bye
        printf_debug("Invalid response %s", response.c_str());
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Join_State::name() const {
    return "Join_State";
}

// ------------ End_State ------------

void End_State::process_input(FSM& fsm, const std::string& input) {
    (void)input; // input is not used here
    printf_debug("Processing input in End State");
    printf_debug("Sending Bye");
    std::ostringstream bye_stream;
    bye_stream << "BYE FROM " << fsm.client.get_display_name() << "\r\n";
    fsm.client.tcp_send(bye_stream.str());
    printf_debug("Bye sent");
    // todo handle quiting
}

void End_State::process_response(FSM& fsm, const std::string& response) {
    (void)fsm; // not used here
    printf_debug("Processing response in End State"); // todo there should not be anything to process here
    printf_debug("Response: %s", response.c_str());
    // todo handle quitting
}

std::string End_State::name() const {
    return "End_State";
}
