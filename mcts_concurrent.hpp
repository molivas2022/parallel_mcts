/*
 * Module: Concurrent MCTS Data Structures
 *
 * This file defines isolated, thread-safe data structures (ConcurrentNode and
 * ConcurrentNodePool) specifically for Tree Parallelization strategies. 
 * By keeping these distinct from the base Node structures, we avoid injecting 
 * atomic operations and memory bloat (like OpenMP locks) into the baseline 
 * Sequential, Leaf, and Root parallel agents, preserving their pure performance 
 * characteristics for accurate benchmarking.
 *
 * Constraints:
 * - Requires OpenMP for node-level structural locking.
 * - Nodes pre-allocate and initialize their locks to avoid data races during
 * tree expansion, trading slightly slower allocation for runtime safety.
 */

#pragma once

#include "common.hpp"
#include "env.hpp"

#include <vector>
#include <atomic>
#include <iostream>
#include <omp.h>

/* Concurrent Node */

struct ConcurrentNode {
    State state;
    ConcurrentNode* parent;
    Action action;

    std::vector<ConcurrentNode*> children;
    
    // Statistics are atomic to allow lock-free Backpropagation and Virtual Loss
    std::atomic<int> visits;
    std::atomic<double> wins; 
    
    ActionSpace untried_space;
    
    // Lightweight spinlock strictly used to protect structural modifications
    // (e.g., popping from untried_space and pushing to children)
    omp_lock_t lock;

    ConcurrentNode() {
        omp_init_lock(&lock);
        visits.store(0, std::memory_order_relaxed);
        wins.store(0.0, std::memory_order_relaxed);
    }
    
    ~ConcurrentNode() {
        omp_destroy_lock(&lock);
    }

    // Disable copy/move semantics to prevent accidental lock duplication
    ConcurrentNode(const ConcurrentNode&) = delete;
    ConcurrentNode& operator=(const ConcurrentNode&) = delete;
};

/* Concurrent Memory Pool */

struct ConcurrentNodePool {
    std::vector<ConcurrentNode> pool;
    
    // Atomic cursor ensures threads can reserve memory blocks simultaneously
    // without returning the same pointer to multiple threads.
    std::atomic<int> cursor;

    ConcurrentNodePool(size_t capacity) {
        pool = std::vector<ConcurrentNode>(capacity);
        cursor.store(0, std::memory_order_relaxed);
    }
    
    void reset() {
        cursor.store(0, std::memory_order_relaxed); 
    }

    ConcurrentNode* allocate(const State& s, ConcurrentNode* p, Action a) {
        // memory_order_relaxed is sufficient here because the atomic operation 
        // only needs to guarantee a unique index, not cross-thread memory synchronization
        int idx = cursor.fetch_add(1, std::memory_order_relaxed);
        
        if (static_cast<size_t>(idx) >= pool.size()) {
            std::cerr << "CRITICAL ERROR: Concurrent Node pool exhausted!\n";
            exit(1);
        }

        ConcurrentNode* n = &pool[idx];
        
        n->state = s;
        n->parent = p;
        n->action = a;
        n->visits.store(0, std::memory_order_relaxed);
        n->wins.store(0.0, std::memory_order_relaxed);
        n->children.clear(); 
        n->untried_space = get_actions(s);
        
        return n;
    }
};