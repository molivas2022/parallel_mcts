#include "agent.hpp"
#include <cmath>
#include <array>

/*
 * Standard Sequential Action Selection
 * Traverses the tree single-threaded, expands, simulates, and backpropagates.
 */
Action SequentialAgent::next_action(const State& root_state) {
    memory_pool.reset();
    Node* root = memory_pool.allocate(root_state, nullptr, Action{0});

    for (int i = 0; i < simulations; ++i) {
        Node* node = root;
        
        // 1. Selection
        while (node->untried_space.count == 0 && !node->children.empty()) {
            Node* best_child = nullptr;
            double best_score = -1.0;
            for (Node* child : node->children) {
                double exploit = child->wins / child->visits;
                double explore = 1.414 * std::sqrt(std::log(node->visits) / child->visits);
                double score = exploit + explore;
                if (score > best_score) {
                    best_score = score;
                    best_child = child;
                }
            }
            node = best_child;
        }
        
        // 2. Expansion
        if (node->untried_space.count > 0 && node->state.winner == Player::None) {
            std::uniform_int_distribution<int> dist(0, node->untried_space.count - 1);
            int idx = dist(eng); 
            Action action = node->untried_space.actions[idx];
            
            node->untried_space.actions[idx] = node->untried_space.actions[node->untried_space.count - 1];
            node->untried_space.count--;
            
            State next_s = node->state;
            next_state(next_s, action);
            
            Node* child = memory_pool.allocate(next_s, node, action);
            node->children.push_back(child);
            node = child;
        }
        
        // 3. Simulation
        State sim_state = node->state;
        while (sim_state.winner == Player::None) {
            ActionSpace space = get_actions(sim_state);
            if (space.count == 0) break;
            std::uniform_int_distribution<int> dist(0, space.count - 1);
            next_state(sim_state, space.actions[dist(eng)]); 
        }
        Player winner = sim_state.winner;
        
        // 4. Backpropagation
        Node* curr = node;
        while (curr != nullptr) {
            curr->visits++;
            if (curr->parent != nullptr && winner == curr->parent->state.turn) {
                curr->wins += 1.0;
            }
            curr = curr->parent;
        }
    }
    
    Action best_action{0};
    int max_visits = -1;
    for (Node* child : root->children) {
        if (child->visits > max_visits) {
            max_visits = child->visits;
            best_action = child->action;
        }
    }
    return best_action;
}

/*
 * Oracle Evaluation Method
 * Runs a massive 100k simulation MCTS loop, mapping every move index to a 
 * continuous [0.0, 1.0] confidence score based on the proportion of visits 
 * it received relative to the absolute best move.
 */
std::array<double, u_SIZE> SequentialAgent::get_action_scores(const State& root_state) {
    memory_pool.reset();
    Node* root = memory_pool.allocate(root_state, nullptr, Action{0});

    for (int i = 0; i < simulations; ++i) {
        Node* node = root;
        
        // Selection
        while (node->untried_space.count == 0 && !node->children.empty()) {
            Node* best_child = nullptr;
            double best_score = -1.0;
            for (Node* child : node->children) {
                double exploit = child->wins / child->visits;
                double explore = 1.414 * std::sqrt(std::log(node->visits) / child->visits);
                double score = exploit + explore;
                if (score > best_score) {
                    best_score = score;
                    best_child = child;
                }
            }
            node = best_child;
        }
        
        // Expansion
        if (node->untried_space.count > 0 && node->state.winner == Player::None) {
            std::uniform_int_distribution<int> dist(0, node->untried_space.count - 1);
            int idx = dist(eng); 
            Action action = node->untried_space.actions[idx];
            
            node->untried_space.actions[idx] = node->untried_space.actions[node->untried_space.count - 1];
            node->untried_space.count--;
            
            State next_s = node->state;
            next_state(next_s, action);
            
            Node* child = memory_pool.allocate(next_s, node, action);
            node->children.push_back(child);
            node = child;
        }
        
        // Simulation
        State sim_state = node->state;
        while (sim_state.winner == Player::None) {
            ActionSpace space = get_actions(sim_state);
            if (space.count == 0) break;
            std::uniform_int_distribution<int> dist(0, space.count - 1);
            next_state(sim_state, space.actions[dist(eng)]); 
        }
        Player winner = sim_state.winner;
        
        // Backprop
        Node* curr = node;
        while (curr != nullptr) {
            curr->visits++;
            if (curr->parent != nullptr && winner == curr->parent->state.turn) {
                curr->wins += 1.0;
            }
            curr = curr->parent;
        }
    }
    
    std::array<double, u_SIZE> scores;
    scores.fill(0.0);
    
    int max_visits = 0;
    for (Node* child : root->children) {
        if (child->visits > max_visits) {
            max_visits = child->visits;
        }
    }
    
    if (max_visits > 0) {
        for (Node* child : root->children) {
            scores[child->action.move_idx] = static_cast<double>(child->visits) / max_visits;
        }
    }
    
    return scores;
}