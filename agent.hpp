#pragma once

#include "env.hpp"
#include "mcts_common.hpp"

#include <string>
#include <random>
#include <vector>
#include <utility>

/* Base interface */

class Agent {
public:
    virtual ~Agent() = default;
    
    virtual Action next_action(const State& state) = 0;
    
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

    Action next_action(const State& root_state) override;
    
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
    
    std::mt19937 main_eng;  // single random engine, for expansion step
    std::vector<std::mt19937> sim_engines; // random engine for each thread in the simulation step

public:
    LeafParallelAgent(int sims, int threads, size_t pool_size = 100000)
        : name("Leaf"), simulations(sims), num_threads(threads), memory_pool(pool_size), main_eng(std::random_device{}()) 
    {
        std::random_device rd;
        for (int i = 0; i < num_threads; ++i) {
            sim_engines.emplace_back(rd());
        }
    }

    Action next_action(const State& root_state) override;
    
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
    
    // each thread manages its own memory pool and random engine
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

    Action next_action(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};

/* * Note: The following agents utilize the ConcurrentNodePool defined in 
* mcts_concurrent.hpp. We forward-declare or use opaque pointers where 
* necessary to avoid polluting this main header, but for simplicity in 
* this benchmark suite, we will just rely on the implementation files 
* to handle the heavy concurrent includes.
*/

class ConcurrentNodePool; // Forward declaration

/* Lock-Free */
class LockFreeParallelAgent : public Agent {
private:
    std::string name;
    int simulations;
    int num_threads;
    
    // Using a void* here to avoid including mcts_concurrent.hpp in the main header,
    // preserving compilation speed and isolating the OpenMP/Atomic bloat.
    // It will be cast to ConcurrentNodePool* in the cpp file.
    void* memory_pool_ptr; 
    
    std::vector<std::mt19937> thread_engines;

public:
    LockFreeParallelAgent(int sims, int threads, size_t pool_size = 100000);
    ~LockFreeParallelAgent() override;

    Action next_action(const State& root_state) override;
    
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

    Action next_action(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_simulations() const override { return simulations; }
    int get_num_threads() const override { return num_threads; }
};