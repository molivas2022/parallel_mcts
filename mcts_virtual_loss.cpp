/*
 * Virtual Loss implementation:
 *
 * utilizes atomic variables and the virtual loss: during the selection phase, as a thread 
 * descends the tree, it immediately increments the atomic visits counter
 */

#include "agent.hpp"
#include "mcts_concurrent.hpp"

#include <cmath>
#include <omp.h>

VirtualLossParallelAgent::VirtualLossParallelAgent(int sims, int threads, size_t pool_size)
    : name("VirtualLoss"), simulations(sims), num_threads(threads) 
{
    memory_pool_ptr = new ConcurrentNodePool(pool_size);
    std::random_device rd;
    for (int i = 0; i < num_threads; ++i) {
        thread_engines.emplace_back(rd());
    }
}

VirtualLossParallelAgent::~VirtualLossParallelAgent() {
    delete static_cast<ConcurrentNodePool*>(memory_pool_ptr);
}

std::array<int, u_SIZE> VirtualLossParallelAgent::get_visit_counts(const State& root_state) {
    auto* pool = static_cast<ConcurrentNodePool*>(memory_pool_ptr);
    pool->reset();
    
    ConcurrentNode* root = pool->allocate(root_state, nullptr, Action{0});

    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        auto& eng = thread_engines[thread_id];
        
        int thread_sims = simulations / num_threads;

        for (int i = 0; i < thread_sims; ++i) {
            ConcurrentNode* node = root;
            
            // Virtual loss
            root->visits.fetch_add(1, std::memory_order_relaxed);
            
            // Selection
            while (node->untried_space.count == 0 && !node->children.empty()) {
                ConcurrentNode* best_child = nullptr;
                double best_score = -1.0;
                
                int parent_visits = node->visits.load(std::memory_order_relaxed);
                
                for (ConcurrentNode* child : node->children) {
                    int child_visits = child->visits.load(std::memory_order_relaxed);
                    double child_wins = child->wins.load(std::memory_order_relaxed);
                    
                    if (child_visits == 0) {
                        best_child = child;
                        break; 
                    }
                    
                    double exploit = child_wins / child_visits;
                    double explore = 1.414 * std::sqrt(std::log(parent_visits) / child_visits);
                    double score = exploit + explore;
                    
                    if (score > best_score) {
                        best_score = score;
                        best_child = child;
                    }
                }
                
                node = best_child;
                // apply VL on the way down
                node->visits.fetch_add(1, std::memory_order_relaxed);
            }
            
            // Expansion
            if (node->untried_space.count > 0 && node->state.winner == Player::None) {
                omp_set_lock(&node->lock);
                
                ConcurrentNode* new_child = nullptr;
                
                if (node->untried_space.count > 0) {
                    std::uniform_int_distribution<int> dist(0, node->untried_space.count - 1);
                    int idx = dist(eng); 
                    Action action = node->untried_space.actions[idx];
                    
                    node->untried_space.actions[idx] = node->untried_space.actions[node->untried_space.count - 1];
                    node->untried_space.count--;
                    
                    State next_s = node->state;
                    next_state(next_s, action);
                    
                    new_child = pool->allocate(next_s, node, action);
                    node->children.push_back(new_child);
                }
                
                omp_unset_lock(&node->lock);
                
                if (new_child != nullptr) {
                    node = new_child;
                    // apply VL on the expanded child
                    node->visits.fetch_add(1, std::memory_order_relaxed);
                }
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
            
            // Backpropagation
            ConcurrentNode* curr = node;
            while (curr != nullptr) {
                // we do NOT need to increment the visits
                if (curr->parent != nullptr) {
                    if (winner == curr->parent->state.turn) {
                        curr->wins.fetch_add(1.0, std::memory_order_relaxed);
                    }
                }
                curr = curr->parent;
            }
        }
    }
    
    std::array<int, u_SIZE> total_visits;
    total_visits.fill(0);
    
    for (ConcurrentNode* child : root->children) {
        total_visits[child->action.move_idx] = child->visits.load(std::memory_order_relaxed);
    }
    
    return total_visits;
}