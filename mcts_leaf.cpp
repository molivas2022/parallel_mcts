#include "agent.hpp"

#include <cmath>
#include <omp.h>

std::array<int, u_SIZE> LeafParallelAgent::get_visit_counts(const State& root_state) {
    memory_pool.reset();
    Node* root = memory_pool.allocate(root_state, nullptr, Action{0});

    int loop_iterations = simulations / num_threads;

    for (int i = 0; i < loop_iterations; ++i) {
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
            int idx = dist(main_eng); 
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
        int first_wins = 0;
        int second_wins = 0;

        #pragma omp parallel for num_threads(num_threads) reduction(+:first_wins, second_wins)
        for (int p = 0; p < num_threads; ++p) {
            int thread_id = omp_get_thread_num();
            auto& sim_eng = sim_engines[thread_id]; 
            
            State sim_state = node->state;
            while (sim_state.winner == Player::None) {
                ActionSpace space = get_actions(sim_state);
                if (space.count == 0) break;
                std::uniform_int_distribution<int> dist(0, space.count - 1);
                next_state(sim_state, space.actions[dist(sim_eng)]);
            }
            
            if (sim_state.winner == Player::First) {
                first_wins++;
            } else if (sim_state.winner == Player::Second) {
                second_wins++;
            }
        }
        
        // Backprop
        Node* curr = node;
        while (curr != nullptr) {
            curr->visits += num_threads;
            if (curr->parent != nullptr) {
                if (curr->parent->state.turn == Player::First) {
                    curr->wins += first_wins;
                } else if (curr->parent->state.turn == Player::Second) {
                    curr->wins += second_wins;
                }
            }
            curr = curr->parent;
        }
    }
    
    std::array<int, u_SIZE> total_visits;
    total_visits.fill(0);
    
    for (auto* child : root->children) {
        total_visits[child->action.move_idx] = child->visits; 
    }
    
    return total_visits;
}