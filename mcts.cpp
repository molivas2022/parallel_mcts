#pragma once

#include <vector>
#include <memory>
#include <random>

#include "common.cpp"
#include "env.cpp"

/* Auxiliar structures */

struct Node {
    State state;
    Node* parent;
    Action action;

    std::vector<std::unique_ptr<Node>> children;
    int visits;
    double wins; 
    std::vector<Action> untried_actions;

    Node(const State& s, Node* p, Action a) 
        : state(s), parent(p), action(a), visits(0), wins(0.0) {
        ActionSpace space = get_actions(s);
        untried_actions.reserve(space.count);
        for (u i = 0; i < space.count; ++i) {
            untried_actions.push_back(space.actions[i]);
        }
    }
};

/* MCTS */

// NOTE: it creates the entire mcts tree from scratch each step
static Action get_mcts_action(const State& root_state, int iterations) {
    auto root = std::make_unique<Node>(root_state, nullptr, Action{0});
    
    static thread_local std::random_device rd;
    static thread_local std::mt19937 eng(rd());

    for (int i = 0; i < iterations; ++i) {
        Node* node = root.get();
        
        // Selection
        while (node->untried_actions.empty() && !node->children.empty()) {
            Node* best_child = nullptr;
            double best_score = -1.0;
            for (auto& child : node->children) {
                double exploit = child->wins / child->visits;
                double explore = 1.414 * std::sqrt(std::log(node->visits) / child->visits);
                double score = exploit + explore;
                if (score > best_score) {
                    best_score = score;
                    best_child = child.get();
                }
            }
            node = best_child;
        }
        
        // Expansion
        if (!node->untried_actions.empty() && node->state.winner == Player::None) {
            std::uniform_int_distribution<int> dist(0, node->untried_actions.size() - 1);
            int idx = dist(eng);
            Action action = node->untried_actions[idx];
            
            node->untried_actions[idx] = node->untried_actions.back();
            node->untried_actions.pop_back();
            
            State next_s = next_state(node->state, action);
            node->children.push_back(std::make_unique<Node>(next_s, node, action));
            node = node->children.back().get();
        }
        
        // Simulation
        State sim_state = node->state;
        while (sim_state.winner == Player::None) {
            ActionSpace space = get_actions(sim_state);
            if (space.count == 0) break;
            std::uniform_int_distribution<int> dist(0, space.count - 1);
            sim_state = next_state(sim_state, space.actions[dist(eng)]);
        }
        Player winner = sim_state.winner;
        
        // Backprop
        Node* curr = node;
        while (curr != nullptr) {
            curr->visits++;
            if (curr->parent != nullptr) {
                if (winner == curr->parent->state.turn) {
                    curr->wins += 1.0;
                }
            }
            curr = curr->parent;
        }
    }
    
    Action best_action;
    int max_visits = -1;
    for (auto& child : root->children) {
        if (child->visits > max_visits) {
            max_visits = child->visits;
            best_action = child->action;
        }
    }
    return best_action;
}