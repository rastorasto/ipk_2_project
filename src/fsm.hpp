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
    std::string create_auth_message(std::string user_name, std::string display_name, std::string secret) const;
    std::string create_bye_message(std::string display_name, std::istringstream& stream) const;
    std::string create_msg_message(std::string display_name, std::string message_content) const;
    std::string create_join_message(std::string display_name, std::string channel_name) const;
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

    void handle_sigint();

    tcp_client& client;
private:
    std::unique_ptr<State> state;
    static FSM* fsm_instance; // needed for SIGINT handling
};
