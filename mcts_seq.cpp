#include "agent.hpp"
#include <cmath>
#include <array>

std::array<int, u_SIZE> SequentialAgent::get_visit_counts(const State& root_state) {
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
    
    std::array<int, u_SIZE> total_visits;
    total_visits.fill(0);
    
    for (Node* child : root->children) {
        total_visits[child->action.move_idx] = child->visits;
    }
    
    return total_visits;
}