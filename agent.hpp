/*
 * Defines the core interface for all mcts agents.
 */

#pragma once

#include "env.hpp"
#include "mcts_common.hpp"

#include <string>
#include <random>
#include <vector>
#include <array>
#include <utility>
#include <thread>

/* Base interface */

class Agent {
public:
    virtual ~Agent() = default;
    
    // core search method to be implemented by all parallel approaches
    virtual std::array<int, u_SIZE> get_visit_counts(const State& state) = 0;
    
    // standard action selection for match pipelines
    Action next_action(const State& state) {
        auto visits = get_visit_counts(state);
        int max_visits = -1;
        u best_move_idx = 0;
        
        for (int i = 0; i < u_SIZE; ++i) {
            if (visits[i] > max_visits) {
                max_visits = visits[i];
                best_move_idx = static_cast<u>(i);
            }
        }
        return Action{best_move_idx};
    }
    
    // continuous distribution evaluation for oracle pipelines
    std::array<double, u_SIZE> get_action_scores(const State& state) {
        auto visits = get_visit_counts(state);
        std::array<double, u_SIZE> scores;
        scores.fill(0.0);
        
        int max_visits = 0;
        for (int v : visits) {
            if (v > max_visits) {
                max_visits = v;
            }
        }
        
        if (max_visits > 0) {
            for (int i = 0; i < u_SIZE; ++i) {
                scores[i] = static_cast<double>(visits[i]) / max_visits;
            }
        }
        return scores;
    }
    
    virtual std::string get_name() const = 0;
    virtual int get_simulations() const = 0;
    virtual int get_num_threads() const = 0;
};

/* Concrete implementations */

/* Sequential */
class SequentialAgent : public Agent {
private:
    std::string name;
    int simulations;
    NodePool memory_pool;
    std::mt19937 eng;

public:
    SequentialAgent(int sims, size_t pool_size = 100000)
        : name("Sequential"), simulations(sims), memory_pool(pool_size), eng(std::random_device{}()) {}

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return 1; }
};

/* Leaf */
class LeafParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    NodePool memory_pool;
    
    std::mt19937 main_eng;
    std::vector<std::mt19937> sim_engines;

public:
    LeafParallelAgent(int sims, int threads, size_t pool_size = 100000)
        : name("Leaf"), simulations(sims), num_threads(threads), memory_pool(pool_size), main_eng(std::random_device{}()) 
    {
        std::random_device rd;
        for (int i = 0; i < num_threads; ++i) {
            sim_engines.emplace_back(rd());
        }
    }

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};

/* Root */
class RootParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    
    std::vector<NodePool> memory_pools;
    std::vector<std::mt19937> thread_engines;

public:
    RootParallelAgent(int sims, int threads, size_t pool_size_per_thread = 100000)
        : name("Root"), simulations(sims), num_threads(threads) 
    {
        std::random_device rd;
        for (int i = 0; i < num_threads; ++i) {
            memory_pools.emplace_back(pool_size_per_thread);
            thread_engines.emplace_back(rd());
        }
    }

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};

class ConcurrentNodePool;

/* Lock-Free */
class LockFreeParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    void* memory_pool_ptr; 
    std::vector<std::mt19937> thread_engines;

public:
    LockFreeParallelAgent(int sims, int threads, size_t pool_size = 100000);
    ~LockFreeParallelAgent() override;

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};

/* Virtual Loss */
class VirtualLossParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    void* memory_pool_ptr; 
    std::vector<std::mt19937> thread_engines;

public:
    VirtualLossParallelAgent(int sims, int threads, size_t pool_size = 100000);
    ~VirtualLossParallelAgent() override;

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};

/* WU-UCT */
class WuUctParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    void* memory_pool_ptr; 

public:
    WuUctParallelAgent(int sims, int threads, size_t pool_size = 100000);
    ~WuUctParallelAgent() override;

    std::array<int, u_SIZE> get_visit_counts(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};