/*
 * Module: Lock-Free MCTS Agent
 *
 * This implementation tests the limits of lock-free heuristic search. It applies
 * a strict OpenMP spinlock EXCLUSIVELY to the tree topology (node expansion) to 
 * prevent memory corruption and segmentation faults. 
 *
 * However, the Backpropagation and Selection phases are intentionally "racy". 
 * Threads load, modify, and store the visits and wins variables without atomic 
 * guarantees (using an explicit read-modify-write pattern on the atomics with 
 * memory_order_relaxed). This will result in dropped increments ("lost updates") 
 * when threads collide. The goal is to see if the sheer volume of simulations 
 * gained by removing synchronization overhead outweighs the statistical degradation 
 * caused by the race conditions.
 */

#include "agent.hpp"
#include "mcts_concurrent.hpp"

#include <cmath>
#include <omp.h>

LockFreeParallelAgent::LockFreeParallelAgent(int sims, int threads, size_t pool_size)
    : name("LockFree"), simulations(sims), num_threads(threads) 
{
    memory_pool_ptr = new ConcurrentNodePool(pool_size);
    std::random_device rd;
    for (int i = 0; i < num_threads; ++i) {
        thread_engines.emplace_back(rd());
    }
}

LockFreeParallelAgent::~LockFreeParallelAgent() {
    delete static_cast<ConcurrentNodePool*>(memory_pool_ptr);
}

Action LockFreeParallelAgent::next_action(const State& root_state) {
    auto* pool = static_cast<ConcurrentNodePool*>(memory_pool_ptr);
    pool->reset();
    
    ConcurrentNode* root = pool->allocate(root_state, nullptr, Action{0});

    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        auto& eng = thread_engines[thread_id];
        
        // Distribute simulations evenly across threads
        int thread_sims = simulations / num_threads;

        for (int i = 0; i < thread_sims; ++i) {
            ConcurrentNode* node = root;
            
            // 1. Selection (Racy reads)
            while (node->untried_space.count == 0 && !node->children.empty()) {
                ConcurrentNode* best_child = nullptr;
                double best_score = -1.0;
                
                // We use memory_order_relaxed because we accept stale data.
                int parent_visits = node->visits.load(std::memory_order_relaxed);
                
                for (ConcurrentNode* child : node->children) {
                    int child_visits = child->visits.load(std::memory_order_relaxed);
                    double child_wins = child->wins.load(std::memory_order_relaxed);
                    
                    if (child_visits == 0) {
                        best_child = child;
                        break; // Immediately explore unvisited nodes
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
            }
            
            // 2. Expansion (Strictly Protected Topology)
            // We MUST lock here because std::vector reallocation or concurrent 
            // array popping will cause a segfault.
            if (node->untried_space.count > 0 && node->state.winner == Player::None) {
                omp_set_lock(&node->lock);
                
                ConcurrentNode* new_child = nullptr;
                
                // Double-check condition after acquiring lock
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
                
                // Unlock the parent BEFORE moving down the tree
                omp_unset_lock(&node->lock);
                
                if (new_child != nullptr) {
                    node = new_child;
                }
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
            
            // 4. Backpropagation (Intentional Data Races)
            ConcurrentNode* curr = node;
            while (curr != nullptr) {
                // EXPLICIT RACE CONDITION: 
                // Load -> Local Add -> Store. 
                // If two threads execute the Load simultaneously, one addition is permanently lost.
                // This simulates a purely lock-free approach without triggering undefined behavior.
                int current_visits = curr->visits.load(std::memory_order_relaxed);
                curr->visits.store(current_visits + 1, std::memory_order_relaxed);
                
                if (curr->parent != nullptr) {
                    if (winner == curr->parent->state.turn) {
                        double current_wins = curr->wins.load(std::memory_order_relaxed);
                        curr->wins.store(current_wins + 1.0, std::memory_order_relaxed);
                    }
                }
                curr = curr->parent;
            }
        }
    }
    
    // Select best move based on raw visits at the root
    Action best_action{0};
    int max_visits = -1;
    for (ConcurrentNode* child : root->children) {
        int v = child->visits.load(std::memory_order_relaxed);
        if (v > max_visits) {
            max_visits = v;
            best_action = child->action;
        }
    }
    return best_action;
}