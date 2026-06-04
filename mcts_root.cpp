#include "agent.hpp"

#include <cmath>
#include <omp.h>
#include <array>

Action RootParallelAgent::next_action(const State& root_state) {
    int loop_iterations = simulations / num_threads;
    
    // Root node of each tree
    std::vector<Node*> thread_roots(num_threads, nullptr);

    // Parallel section, each thread runs MCTS
    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        auto& pool = memory_pools[thread_id];
        auto& eng = thread_engines[thread_id];

        pool.reset();
        Node* root = pool.allocate(root_state, nullptr, Action{0});
        thread_roots[thread_id] = root;

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
                int idx = dist(eng); 
                Action action = node->untried_space.actions[idx];
                
                node->untried_space.actions[idx] = node->untried_space.actions[node->untried_space.count - 1];
                node->untried_space.count--;
                
                State next_s = node->state;
                next_state(next_s, action);
                
                Node* child = pool.allocate(next_s, node, action);
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
                if (curr->parent != nullptr) {
                    if (winner == curr->parent->state.turn) {
                        curr->wins += 1.0;
                    }
                }
                curr = curr->parent;
            }
        }
    }

    // Aggregation of all trees
    std::array<int, BOARD_SIZE> total_visits = {0};
    for (Node* root : thread_roots) {
        for (Node* child : root->children) {
            total_visits[child->action.move_idx] += child->visits;
        }
    }

    // Selection of the most visited move
    Action best_action{0};
    int max_visits = -1;

    for (u i = 0; i < BOARD_SIZE; ++i) {
        if (total_visits[i] > max_visits) {
            max_visits = total_visits[i];
            best_action = Action{i};
        }
    }
    
    return best_action;
}