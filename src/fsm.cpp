#include "fsm.hpp"
#include <sstream>
#include <iostream>

#define DEBUG_PRINT
#include "macro.hpp"

FSM::FSM() : state(std::make_unique<Start_State>()) {}

void FSM::process_client_input(const std::string& input) {
    state->process_input(*this, input);
}

void FSM::change_state(std::unique_ptr<State> new_state) {
    printf_debug("Changing state from %s to %s", state->name().c_str(), new_state->name().c_str());
    state = std::move(new_state);
}

// ------------ Start_State ------------

void Start_State::enter(FSM& fsm) {
    printf_debug("Entering Start State");
}

void Start_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Start State");
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

std::string Start_State::name() const {
    return "Start_State";
}

// ------------ Auth_State ------------

void Auth_State::enter(FSM& fsm) {
    printf_debug("Entering Auth State");
}

void Auth_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Auth State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    printf_debug("Command %s received", command.c_str());

    if(command == "REPLY"){
        // I am leaving change state in /auth for now so i can debug it better and see in what state i am in
        printf_debug("Successful authentication");
        fsm.change_state(std::make_unique<Open_State>());
    } else if (command == "/auth") { // !REPLY
        fsm.change_state(std::make_unique<Auth_State>());
    } else if (command == "/bye") { // ERR, BYE, MSG
        // i am leaving /bye as err and others for easier debugging and testing now
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

std::string Auth_State::name() const {
    return "Auth_State";
}

// ----------- Open_State ------------

void Open_State::enter(FSM& fsm) {
    printf_debug("Entering Open State");
}

void Open_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Open State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    printf_debug("Command: %s", command.c_str());

    if(command == "/msg"){ // MSG
        // fsm.change_state(std::make_unique<Open_State>());
        // staying in the same state
        printf_debug("Processing message");
    } else if (command == "/join") { // JOIN
        fsm.change_state(std::make_unique<Join_State>());
    } else if (command == "/bye") { // ERR, BYE, MSG
        // i am leaving /bye as err and others for easier debugging and testing now
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

std::string Open_State::name() const {
    return "Open_State";
}

// ----------- Join_State ------------

void Join_State::enter(FSM& fsm) {
    printf_debug("Entering Join State");
}

void Join_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in Join State");
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    printf_debug("Command: %s", command.c_str());

    if(command == "MSG"){ // MSG
        printf_debug("Server replied msg");
    } else if (command == "REPLY") { // *REPLY
        fsm.change_state(std::make_unique<Open_State>());
    } else if (command == "/bye") { // ERR, BYE, MSG
        // i am leaving /bye as err and others for easier debugging and testing now
        fsm.change_state(std::make_unique<End_State>());
    } else {
        printf_debug("Invalid command");
    }
}

std::string Join_State::name() const {
    return "Join_State";
}

// ------------ End_State ------------

void End_State::enter(FSM& fsm) {
    printf_debug("Entering End State");
}

// todo ? delete wont be used since end state is final state
void End_State::process_input(FSM& fsm, const std::string& input) {
    printf_debug("Processing input in End State");
}

std::string End_State::name() const {
    return "End_State";
}
