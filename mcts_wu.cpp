/*
 * Module: WU-UCT MCTS Agent (OpenMP Task Parallelization)
 *
 * This implements the Pipeline architecture using OpenMP Tasking. 
 * An OpenMP parallel region wakes up a team of threads. A single Master 
 * thread traverses the MCTS tree, performing Selection and Expansion safely
 * without locks. 
 * 
 * It tracks unobserved samples (O_s) to retain exploration diversity. 
 * When a node is expanded, the Master spawns an OpenMP task. The OpenMP 
 * runtime automatically distributes these simulation tasks to the idle 
 * worker threads in the team, which asynchronously execute the rollout 
 * and perform the Complete Update via atomic operations.
 */

#include "agent.hpp"
#include "mcts_wu.hpp"

#include <cmath>
#include <random>
#include <omp.h>
#include <vector>

WuUctParallelAgent::WuUctParallelAgent(int sims, int threads, size_t pool_size)
    : name("WuUct"), simulations(sims), num_threads(threads) 
{
    memory_pool_ptr = new WuNodePool(pool_size);
}

WuUctParallelAgent::~WuUctParallelAgent() {
    delete static_cast<WuNodePool*>(memory_pool_ptr);
}

std::array<int, u_SIZE> WuUctParallelAgent::get_visit_counts(const State& root_state) {
    auto* pool = static_cast<WuNodePool*>(memory_pool_ptr);
    pool->reset();
    WuNode* root = pool->allocate(root_state, nullptr, Action{0});

    // Pre-allocate random engines for all threads to avoid thread_local overhead
    // and OS entropy locks during the heavy task phase.
    std::vector<std::mt19937> sim_engines(num_threads);
    std::random_device rd;
    for (int i = 0; i < num_threads; ++i) {
        sim_engines[i].seed(rd());
    }

    // Wake up the team of threads
    #pragma omp parallel num_threads(num_threads)
    {
        // Restrict tree traversal to a single Master thread
        #pragma omp single
        {
            // The master thread gets its own engine for the expansion phase
            std::mt19937 master_eng(rd());

            for (int i = 0; i < simulations; ++i) {
                WuNode* node = root;
                
                // 1. Selection (Master Only)
                while (node->untried_space.count == 0 && !node->children.empty()) {
                    WuNode* best_child = nullptr;
                    double best_score = -1.0;
                    
                    int parent_visits = node->visits.load(std::memory_order_relaxed);
                    int parent_unobs = node->unobserved.load(std::memory_order_relaxed);
                    int parent_total = parent_visits + parent_unobs;
                    
                    for (WuNode* child : node->children) {
                        int child_visits = child->visits.load(std::memory_order_relaxed);
                        int child_unobs = child->unobserved.load(std::memory_order_relaxed);
                        int child_total = child_visits + child_unobs;
                        
                        if (child_total == 0) {
                            best_child = child;
                            break; 
                        }
                        
                        double child_wins = child->wins.load(std::memory_order_relaxed);
                        double exploit = child_visits > 0 ? (child_wins / child_visits) : 0.0;
                        double explore = 1.414 * std::sqrt(std::log(parent_total) / child_total);
                        double score = exploit + explore;
                        
                        if (score > best_score) {
                            best_score = score;
                            best_child = child;
                        }
                    }
                    node = best_child;
                }
                
                // 2. Expansion (Master Only)
                if (node->untried_space.count > 0 && node->state.winner == Player::None) {
                    std::uniform_int_distribution<int> dist(0, node->untried_space.count - 1);
                    int idx = dist(master_eng); 
                    Action action = node->untried_space.actions[idx];
                    
                    node->untried_space.actions[idx] = node->untried_space.actions[node->untried_space.count - 1];
                    node->untried_space.count--;
                    
                    State next_s = node->state;
                    next_state(next_s, action);
                    
                    WuNode* child = pool->allocate(next_s, node, action);
                    node->children.push_back(child);
                    node = child;
                }
                
                // 3. Incomplete Update (Master Only)
                WuNode* curr_inc = node;
                while (curr_inc != nullptr) {
                    curr_inc->unobserved.fetch_add(1, std::memory_order_relaxed);
                    curr_inc = curr_inc->parent;
                }
                
                // 4. Dispatch Simulation Task to Workers
                // firstprivate safely passes the node pointer, shared safely exposes the engine array
                #pragma omp task firstprivate(node) shared(sim_engines)
                {
                    // Access the pre-allocated engine using the worker's native thread ID
                    int thread_id = omp_get_thread_num();
                    auto& worker_eng = sim_engines[thread_id];
                    
                    State sim_state = node->state;
                    
                    // Simulation Phase (Heavy workload executed by workers)
                    while (sim_state.winner == Player::None) {
                        ActionSpace space = get_actions(sim_state);
                        if (space.count == 0) break;
                        std::uniform_int_distribution<int> dist(0, space.count - 1);
                        next_state(sim_state, space.actions[dist(worker_eng)]);
                    }
                    Player winner = sim_state.winner;
                    
                    // Complete Update (Backpropagation executed by workers)
                    WuNode* curr_comp = node;
                    while (curr_comp != nullptr) {
                        curr_comp->unobserved.fetch_sub(1, std::memory_order_relaxed);
                        curr_comp->visits.fetch_add(1, std::memory_order_relaxed);
                        
                        if (curr_comp->parent != nullptr && winner == curr_comp->parent->state.turn) {
                            // C++17 safe atomic double addition via CAS
                            double current_wins = curr_comp->wins.load(std::memory_order_relaxed);
                            while (!curr_comp->wins.compare_exchange_weak(current_wins, current_wins + 1.0, 
                                                                          std::memory_order_relaxed, 
                                                                          std::memory_order_relaxed)) {}
                        }
                        curr_comp = curr_comp->parent;
                    }
                } // End of task
            }
            
            // Wait for all dispatched simulation tasks to complete before choosing a move
            #pragma omp taskwait
        } // End of single Master thread
    } // End of parallel region
    
    std::array<int, u_SIZE> total_visits;
    total_visits.fill(0);
    
    for (auto* child : root->children) {
        total_visits[child->action.move_idx] = child->visits.load(std::memory_order_relaxed);
    }
    
    return total_visits;
}