#pragma once
#include <iostream>
#include <memory>
#include "tcpclient.hpp"


// Forward declaration of FSM
struct FSM;

struct State {
    virtual void process_input(FSM& fsm, const std::string& input) = 0;
    virtual void process_response(FSM& fsm, const std::string& response) = 0;
    virtual ~State() = default;
    virtual std::string name() const = 0;
};

struct Start_State : State {
    void process_input(FSM& fsm, const std::string& input) override;
    void process_response(FSM& fsm, const std::string& response) override;
    std::string name() const override;
};

struct Auth_State : Start_State {
    void process_input(FSM& fsm, const std::string& input) override;
    void process_response(FSM& fsm, const std::string& response) override;
    std::string name() const override;
};

struct End_State : State {
    void process_input(FSM& fsm, const std::string& input) override;
    void process_response(FSM& fsm, const std::string& response) override;
    std::string name() const override;
};

struct Open_State : State {
    void process_input(FSM& fsm, const std::string& input) override;
    void process_response(FSM& fsm, const std::string& response) override;
    std::string name() const override;
};

struct Join_State : State {
    void process_input(FSM& fsm, const std::string& input) override;
    void process_response(FSM& fsm, const std::string& response) override;
    std::string name() const override;
};

struct FSM {
    FSM(tcp_client& client);
    void process_client_input(const std::string& input);
    void process_server_response(const std::string& response);
    void change_state(std::unique_ptr<State> new_state);
    tcp_client& client;
private:
    std::unique_ptr<State> state;
};
