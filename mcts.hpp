#pragma once

#include "common.hpp"
#include "env.hpp"

#include <vector>
#include <memory>

/* Auxiliar structures */

struct Node {
    State state;
    Node* parent;
    Action action;

    std::vector<std::unique_ptr<Node>> children;
    int visits;
    double wins; 
    std::vector<Action> untried_actions;

    Node(const State& s, Node* p, Action a);
};

/* MCTS */

Action get_mcts_action(const State& root_state, int iterations);