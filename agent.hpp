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
    virtual int get_iters() const = 0;
    virtual int get_num_threads() const = 0;
};

/* Concrete implementations */

/* Sequential */
class SequentialAgent : public Agent {
private:
    std::string name;
    int iterations;
    NodePool memory_pool;
    std::mt19937 eng;

public:
    SequentialAgent(int iters, size_t pool_size = 100000)
        : name("Sequential"), iterations(iters), memory_pool(pool_size), eng(std::random_device{}()) {}

    Action next_action(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_iters() const override { return iterations; }
    int get_num_threads() const override { return 1; }
};

/* Leaf */
class LeafParallelAgent : public Agent {
private:
    std::string name;
    int iterations;
    int num_threads;
    NodePool memory_pool;
    
    std::mt19937 main_eng; 
    std::vector<std::mt19937> sim_engines; 

public:
    LeafParallelAgent(int iters, int threads, size_t pool_size = 200000)
        : name("Leaf"), iterations(iters), num_threads(threads), memory_pool(pool_size), main_eng(std::random_device{}()) 
    {
        std::random_device rd;
        for (int i = 0; i < num_threads; ++i) {
            sim_engines.emplace_back(rd());
        }
    }

    Action next_action(const State& root_state) override;
    
    std::string get_name() const override { return name; }
    int get_iters() const override { return iterations; }
    int get_num_threads() const override { return num_threads; }
};