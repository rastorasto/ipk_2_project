#pragma once
#include <iostream>
#include <memory>

// Forward declaration of FSM
struct FSM;

struct State {
    virtual void enter(FSM& fsm) = 0;
    virtual void process_input(FSM& fsm, const std::string& input) = 0;
    virtual ~State() = default;
    virtual std::string name() const = 0;
};

struct Start_State : State {
    void enter(FSM& fsm) override;
    void process_input(FSM& fsm, const std::string& input) override;
    std::string name() const override;
};

struct Auth_State : Start_State {
    void process_input(FSM& fsm, const std::string& input) override;
    void enter(FSM& fsm) override;
    std::string name() const override;
};

struct End_State : State {
    void enter(FSM& fsm) override;
    void process_input(FSM& fsm, const std::string& input) override;
    std::string name() const override;
};

struct Open_State : State {
    void enter(FSM& fsm) override;
    void process_input(FSM& fsm, const std::string& input) override;
    std::string name() const override;
};

struct Join_State : State {
    void enter(FSM& fsm) override;
    void process_input(FSM& fsm, const std::string& input) override;
    std::string name() const override;
};

struct FSM {
    FSM();
    void process_client_input(const std::string& input);
    void change_state(std::unique_ptr<State> new_state);
private:
    std::unique_ptr<State> state;
};
