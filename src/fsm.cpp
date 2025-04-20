#include "fsm.hpp"
#include <sstream>
#include <iostream>

#include "macro.hpp"
#include "tcpclient.hpp"

#include <csignal>

const size_t MAX_MESSAGE_LENGTH = 60000;
FSM* FSM::fsm_instance = nullptr;

FSM::FSM(tcp_client& client) : client(client), state(std::make_unique<Start_State>()){
    fsm_instance = this;
    // Initialize signal handling for SIGINT
    std::signal(SIGINT, [](int signal_number) {
        (void)signal_number;
        printf_debug("Received SIGINT signal");
        fsm_instance->handle_bye();
    });
}

// Calls the current state's process_input method
void FSM::process_client_input(const std::string& input) {
    state->process_input(*this, input);
}

// Calls the current state's process_response method
void FSM::process_server_response(const std::string& response) {
    state->process_response(*this, response);
}

// Changes the current state to a new one
void FSM::change_state(std::unique_ptr<State> new_state) {
    printf_debug("Changing state from %s to %s", state->name().c_str(), new_state->name().c_str());

    state = std::move(new_state);

    // If the new state is End_State it is not expected to receive user or server input therefore I call the process_input method directly
    if(state->name()== "End_State"){
        state->process_input(*this, "");
    }
}

void FSM::handle_bye() {
    printf_debug("Received sigint or error");
    change_state(std::make_unique<End_State>());
}

// Function for creating a message
std::string State::create_msg_message(std::string display_name, std::string message_content) const{

    if (message_content.size() > MAX_MESSAGE_LENGTH) { // Checks the length of the message content, if it is greater it is cut off
        message_content = message_content.substr(0, MAX_MESSAGE_LENGTH);
    }

    std::ostringstream token_stream;
    token_stream << "MSG FROM " << display_name << " IS " << message_content << "\r\n";
    return token_stream.str();
}

// Creates an authentication message
std::string State::create_auth_message(std::string username, std::string display_name, std::string secret) const{
    std::ostringstream token_stream;
    token_stream << "AUTH " << username << " AS " << display_name << " USING " << secret << "\r\n";
    return token_stream.str();
}

// Creates a join message
std::string State::create_join_message(std::string display_name, std::string channel_name) const{

    std::ostringstream token_stream;
    token_stream << "JOIN " << channel_name << " AS " << display_name << "\r\n";
    std::string token = token_stream.str();
    printf_debug("Sending Token %s to Server", token.c_str());
    return token;
}

// ------------ Start_State ------------

// Processes user input in the Start_State
void Start_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Start State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    printf_debug("Input %s", input.c_str());
    printf_debug("Command %s received", command.c_str());


    if (command == "/auth") {
        std::string username, secret, display_name;
        iss >> username >> secret >> display_name;

        std::string leftover;
            iss >> leftover;
        if (!leftover.empty()) {
            printf_debug("Invalid display name — too many arguments.");
            std::cout<< "ERROR: Too many arguments" << std::endl;
            return;
        }

        fsm.client.set_display_name(display_name);

        std::string token = create_auth_message(username, display_name, secret);
        fsm.client.tcp_send(token);

        fsm.change_state(std::make_unique<Auth_State>());
    } else if (command == "/bye") {
        fsm.handle_bye();
        fsm.change_state(std::make_unique<End_State>());
    } else if (command == "/help"){
        printf_debug("/help in Start State");
        std::cout << "Available commands:" << std::endl;
        std::cout << "Use to authenticate /auth <username> <display_name> <secret>" << std::endl;
        std::cout << "Use to disconnect from the server /bye " << std::endl;
        std::cout << "Use to show this help /help" << std::endl;
        std::cout << "You can also disconnect by pressing Ctrl+C" << std::endl;
        std::cout << "After authentication any message that does not start with commands displayed above will be sent as message" << std::endl;
    } else {
        std::cout << "ERROR: Invalid command write /help for commands" << std::endl;
        printf_debug("Invalid command");
    }
}

// Processes server response in the Start_State
void Start_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Start State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;

    std::transform(fsm_name.begin(), fsm_name.end(), fsm_name.begin(), ::toupper); // Capitalizing fsm_name because grammar is case insensitive

    if(fsm_name == "ERR"){
        // err from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        if (!message_content.empty() && message_content.back() == '\r') {
            message_content.pop_back();
        }

        std::cout << "ERROR FROM " << name<< ": " << message_content << std::endl;
        printf_debug("ERROR in start state");
        fsm.change_state(std::make_unique<End_State>());
    } else if(fsm_name == "BYE"){
        // bye from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::cout << "BYE FROM " << name << std::endl;
        printf_debug("BYE FROM in start state");
        fsm.change_state(std::make_unique<Open_State>());
    } else {
        printf_debug("else in start state");
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Start_State::name() const {
    return "Start_State";
}

// ------------ Auth_State ------------

// Processes user input in the Auth_State
void Auth_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Auth State");

    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command %s received", command.c_str());

    if (command == "/auth") {
        std::string username, secret, display_name;
        iss >> username >> secret >> display_name;

        std::string leftover;
            iss >> leftover;
        if (!leftover.empty()) {
            printf_debug("Invalid display name — too many arguments.");
            std::cout<< "ERROR: Too many arguments" << std::endl;
            return;
        }

        fsm.client.set_display_name(display_name);

        std::string token = create_auth_message(username, display_name, secret);

        fsm.client.tcp_send(token);
    } else if (command == "/bye") {
        fsm.handle_bye();
    } else if (command == "/help"){
        printf_debug("/help in Start State");
        std::cout << "Available commands:" << std::endl;
        std::cout << "Use to authenticate /auth <username> <display_name> <secret>" << std::endl;
        std::cout << "Use to disconnect from the server /bye " << std::endl;
        std::cout << "Use to show this help /help" << std::endl;
        std::cout << "You can also disconnect by pressing Ctrl+C" << std::endl;
        std::cout << "After authentication any message that does not start with commands displayed above will be sent as message" << std::endl;
    } else {
        std::cout << "ERROR: Invalid command write /help for commands" << std::endl;
        printf_debug("Invalid command");
    }
}

// Processes server response in the Auth_State
void Auth_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Auth State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;

    std::transform(fsm_name.begin(), fsm_name.end(), fsm_name.begin(), ::toupper); // Capitalizing fsm_name because grammar is case insensitive

    if(fsm_name == "REPLY"){
        std::string status;
        iss >> status;
        std::transform(status.begin(), status.end(), status.begin(), ::toupper); // Capitalizing status because grammar is case insensitive
        std::string is_keyword;
        iss >> is_keyword;
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        if (!message_content.empty() && message_content.back() == '\r') {
            message_content.pop_back();
        }

        if(status == "OK"){
            std::cout << "Action Success: " << message_content << std::endl;
            fsm.change_state(std::make_unique<Open_State>());
        } else if (status == "NOK"){
            std::cout << "Action Failure: " << message_content << std::endl;
        }
        else {
            printf_debug("Should never get here"); // I will leave it here just to be sure but server should always respond with either OK or NOK as written in the assignment (REPLY {"OK"|"NOK"} IS {MessageContent}\r\n)
            std::cout << "ERROR: Invalid server response" << std::endl;
        }
    } else if(fsm_name == "ERR"){
        // err from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        if (!message_content.empty() && message_content.back() == '\r') {
                    message_content.pop_back();
        }
        std::cout << "ERROR FROM " << name<< ": " << message_content << std::endl;
        printf_debug("ERROR in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else if(fsm_name == "BYE"){
        // bye from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::cout << "BYE FROM " << name << std::endl;
        printf_debug("BYE FROM in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else if (fsm_name == "MSG"){
        // msg from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        std::cout << "MSG FROM " << name << ": " << message_content << std::endl;
        std::cout << "ERROR: Message received in auth state" << std::endl;
        std::ostringstream error_message;
        error_message << "ERR FROM " << fsm.client.get_display_name() << " IS " << "Message received in auth state." << "\r\n";
        fsm.client.tcp_send(error_message.str());

        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Auth state else should not get here");
        std::cout << "ERROR: Invalid message received from the server" << std::endl;
        std::ostringstream error_message;
        error_message << "ERR FROM " << fsm.client.get_display_name() << " IS " << "Missing or malformed ERR message." << "\r\n";
        fsm.client.tcp_send(error_message.str());
        printf_debug("Sending error message");
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Auth_State::name() const {
    return "Auth_State";
}

// ----------- Open_State ------------

// Processes user input in the Open_State
void Open_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Open State");

    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command: %s", command.c_str());

    if (command == "/join") { // JOIN
        std::string channel_name;
        iss >> channel_name;

        std::string left_over;
        iss >> left_over;
        if(!left_over.empty()){
            std::cout << "ERROR: Invalid channel name" << std::endl;
            return;
        }

        std::string token = create_join_message(fsm.client.get_display_name(), channel_name);

        fsm.client.tcp_send(token);

        fsm.change_state(std::make_unique<Join_State>());
    } else if (command == "/rename") {
        std::string new_name;
        iss >> new_name;

        std::string left_over;
        iss >> left_over;
        if(!left_over.empty()){
            std::cout << "ERROR: Invalid new name" << std::endl;
            return;
        }
        fsm.client.set_display_name(new_name);

    } else if (command == "/bye") {
        fsm.handle_bye();
    } else if (command == "/help"){
    printf_debug("/help in Start State");
    std::cout << "Available commands:" << std::endl;
    std::cout << "Use to authenticate /auth <username> <display_name> <secret>" << std::endl;
    std::cout << "Use to disconnect from the server /bye " << std::endl;
    std::cout << "Use to show this help /help" << std::endl;
    std::cout << "You can also disconnect by pressing Ctrl+C" << std::endl;
    std::cout << "After authentication any message that does not start with commands displayed above will be sent as message" << std::endl;
    } else if (command == "/auth"){
        std::cout << "ERROR: Already authenticated" << std::endl;
        fsm.handle_bye();
    } else { // msg
        std::string token = create_msg_message(fsm.client.get_display_name(), input);
        printf_debug("Sending Token %s to Server", token.c_str());
        fsm.client.tcp_send(token);
    }
}

// Processes server response in the Open_State
void Open_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Open State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;

    std::transform(fsm_name.begin(), fsm_name.end(), fsm_name.begin(), ::toupper); // Capitalizing fsm_name because grammar is case insensitive

    if(fsm_name == "MSG"){ // MSG / _
        std::stringstream ss(response);
        std::string line;
        // Handle multiple messages in one response
        while (std::getline(ss, line)) {
            // Remove \r from the message
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue; // Just to make sure empty line is not processed

            std::istringstream iss(line);
            std::string tag, from_keyword, name, is_keyword, message_content;
            iss >> tag >> from_keyword >> name >> is_keyword;
            std::transform(tag.begin(), tag.end(), tag.begin(), ::toupper);
            std::transform(from_keyword.begin(), from_keyword.end(), from_keyword.begin(), ::toupper);
            std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
            if (tag != "MSG" || from_keyword != "FROM" || is_keyword != "IS") {
                printf_debug("Invalid grammar");
                std::cout << "ERROR: Received message with invalid grammar" << std::endl;
                continue;
            }
            std::getline(iss >> std::ws, message_content);
            std::cout << name << ": " << message_content << std::endl;
        }
    } else if(fsm_name == "REPLY"){ // *REPLY / ERR

        std::string status;
        iss >> status;
        std::transform(status.begin(), status.end(), status.begin(), ::toupper); // Capitalizing status because grammar is case insensitive
        if(status == "OK"){
            std::string is_keyword;
            iss >> is_keyword;
            std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // should not happen but just to be sure
                std::cout << "ERROR: Received message with invalid grammar" << std::endl;
            }
            std::string message_content;
            std::getline(iss >> std::ws, message_content);
            if (!message_content.empty() && message_content.back() == '\r') {
                message_content.pop_back();
            }
            std::cout << "ERROR: Message received in auth state";
            std::ostringstream error_message;
            error_message << "ERR FROM " << fsm.client.get_display_name() << " IS " << "Reply received in open state." << "\r\n";
            fsm.client.tcp_send(error_message.str());
            fsm.change_state(std::make_unique<End_State>());
        } else if(status == "NOK"){
            std::string is_keyword;
            iss >> is_keyword;
            std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar"); // should not happen but just to be sure
                std::cout << "ERROR: Received message with invalid grammar" << std::endl;
            }
            std::string message_content;
            std::getline(iss >> std::ws, message_content);
            std::cout << "ERROR: Message received in auth state";
            std::ostringstream error_message;
            error_message << "ERR FROM " << fsm.client.get_display_name() << " IS " << "Reply received in open state." << "\r\n";
            fsm.client.tcp_send(error_message.str());

            fsm.change_state(std::make_unique<End_State>());
        } else {
            std::cout << "ERROR: Invalid status " << status << std::endl;
            printf_debug("Invalid status in open state ");
        }
    } else if(fsm_name == "ERR"){ // ERR / _
        // err from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::transform(from_keyword.begin(), from_keyword.end(), from_keyword.begin(), ::toupper);
        std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
        if(from_keyword != "FROM" || is_keyword != "IS"){
            printf_debug("Invalid grammar");
            std::cout << "ERROR: Received message with invalid grammar" << std::endl;
        }
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        if (!message_content.empty() && message_content.back() == '\r') {
            message_content.pop_back();
        }
        std::cout << "ERROR FROM " << name<< ": " << message_content << std::endl;
        printf_debug("ERROR in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else if(fsm_name == "BYE"){ // BYE / _
        // bye from server
        std::string from_keyword;
        iss >> from_keyword;
        std::transform(from_keyword.begin(), from_keyword.end(), from_keyword.begin(), ::toupper);
        if(from_keyword != "FROM"){
            printf_debug("Invalid grammar");
            std::cout << "ERROR: Received message with invalid grammar" << std::endl;
        }
        std::string name;
        iss >> name;
        std::cout << "BYE FROM " << name << std::endl;
        printf_debug("BYE FROM in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else {
        std::cout << "ERROR: Invalid message received from the server" << std::endl;
        std::ostringstream error_message;
        error_message << "ERR FROM " << fsm.client.get_display_name() << " IS " << "Missing or malformed ERR message." << "\r\n";
        fsm.client.tcp_send(error_message.str());
        printf_debug("Sending error message");
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Open_State::name() const {
    return "Open_State";
}

// ----------- Join_State ------------

// Processes user input in the Join_State
void Join_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Join State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    printf_debug("Command: %s", command.c_str());

    if (command == "/bye") { // _ / BYE
        fsm.handle_bye();
    } else {
        printf_debug("Invalid command");
    }
}

// Processes server response in the Join_State
void Join_State::process_response(FSM& fsm, const std::string& response) {
    printf_debug("Processing response in Join State");
    printf_debug("Response: %s", response.c_str());

    std::istringstream iss(response);
    std::string fsm_name;
    iss >> fsm_name;

    std::transform(fsm_name.begin(), fsm_name.end(), fsm_name.begin(), ::toupper); // Capitalizing fsm_name because grammar is case insensitive

    if(fsm_name == "MSG"){ // MSG / _
        std::string from_keyword;
        iss >> from_keyword;
        std::transform(from_keyword.begin(), from_keyword.end(), from_keyword.begin(), ::toupper);
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
        if(from_keyword != "FROM" || is_keyword != "IS"){
            printf_debug("Invalid grammar");
            std::cout << "ERROR: Received message with invalid grammar" << std::endl;
        }
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        std::cout << name << ": " << message_content << std::endl;
    } else if(fsm_name == "REPLY"){ // * REPLY / _
        std::string status;
        iss >> status;
        if(status == "OK"){
            std::string is_keyword;
            iss >> is_keyword;
            std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar");
                std::cout << "ERROR: Received message with invalid grammar" << std::endl;
            }
            std::string message_content;
            std::getline(iss >> std::ws, message_content);
            if (!message_content.empty() && message_content.back() == '\r') {
                message_content.pop_back();
            }
            std::cout << "Action Success: " << message_content << std::endl;
            fsm.change_state(std::make_unique<Open_State>());
        } else if(status == "NOK"){
            std::string is_keyword;
            iss >> is_keyword;
            std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
            if(is_keyword != "IS"){
                printf_debug("Invalid grammar");
                std::cout << "ERROR: Received message with invalid grammar" << std::endl;
            }
            std::string message_content;
            std::getline(iss >> std::ws, message_content);
            if (!message_content.empty() && message_content.back() == '\r') {
                message_content.pop_back();
            }
            std::cout << "Action Failure: " << message_content << std::endl;
            fsm.change_state(std::make_unique<Open_State>());
        } else {
            std::cout << "ERROR: Invalid status " << status << std::endl;
            printf_debug("Invalid status in open state ");
        }
    } else if(fsm_name == "ERR"){ // ERR / _
        // err from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::string is_keyword;
        iss >> is_keyword;
        std::transform(from_keyword.begin(), from_keyword.end(), from_keyword.begin(), ::toupper);
        std::transform(is_keyword.begin(), is_keyword.end(), is_keyword.begin(), ::toupper);
        if(from_keyword != "FROM" || is_keyword != "IS"){
            printf_debug("Invalid grammar");
            std::cout << "ERROR: Received message with invalid grammar" << std::endl;
        }
        std::string message_content;
        std::getline(iss >> std::ws, message_content);
        if (!message_content.empty() && message_content.back() == '\r') {
            message_content.pop_back();
        }
        std::cout << "ERROR FROM " << name << ": " << message_content << std::endl;
        printf_debug("ERROR in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else if(fsm_name == "BYE"){ // BYE / _
        // bye from server
        std::string from_keyword;
        iss >> from_keyword;
        std::string name;
        iss >> name;
        std::cout << "BYE FROM " << name << std::endl;
        printf_debug("BYE FROM in auth state");
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Auth state else should not get here"); // and if it does print error and go to end state
        std::cout << "ERROR: Received invalid message from the server - quitting";
        fsm.change_state(std::make_unique<End_State>());
    }
}

std::string Join_State::name() const {
    return "Join_State";
}

// ------------ End_State ------------

// This is implemented because in base class State it is a virtual method
void End_State::process_input(FSM& fsm, const std::string& input) {
    (void)input; // input is not used here
    printf_debug("Processing input in End State");

    write(fsm.client.pipe_fds[1], "x", 1);
}

// This is called directly because input is not processed in end state
void End_State::process_response(FSM& fsm, const std::string& response) {
    (void)fsm; // not used here
    printf_debug("Response: %s", response.c_str());
    write(fsm.client.pipe_fds[1], "x", 1);
}

std::string End_State::name() const {
    return "End_State";
}
