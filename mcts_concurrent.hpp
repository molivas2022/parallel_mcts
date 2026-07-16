/*
 * This file defines isolated, thread safe data structures specifically for tree parallelization strategies. 
 */

#pragma once

#include "common.hpp"
#include "env.hpp"

#include <vector>
#include <atomic>
#include <iostream>
#include <omp.h>

/* Concurrent node */

struct ConcurrentNode {
    State state;
    ConcurrentNode* parent;
    Action action;

    std::vector<ConcurrentNode*> children;
    
    // atomic statistics allow lock-free
    std::atomic<int> visits;
    std::atomic<double> wins; 
    
    ActionSpace untried_space;
    
    // spinlock used to protect structural modifications
    omp_lock_t lock;

    ConcurrentNode() {
        omp_init_lock(&lock);
        visits.store(0, std::memory_order_relaxed);
        wins.store(0.0, std::memory_order_relaxed);
    }
    
    ~ConcurrentNode() {
        omp_destroy_lock(&lock);
    }

    ConcurrentNode(const ConcurrentNode&) = delete;
    ConcurrentNode& operator=(const ConcurrentNode&) = delete;
};

/* Concurrent memory pool */

struct ConcurrentNodePool {
    std::vector<ConcurrentNode> pool;
    
    // atomic cursor to safe allocation
    std::atomic<int> cursor;

    ConcurrentNodePool(size_t capacity) {
        pool = std::vector<ConcurrentNode>(capacity);
        cursor.store(0, std::memory_order_relaxed);
    }
    
    void reset() {
        cursor.store(0, std::memory_order_relaxed); 
    }

    ConcurrentNode* allocate(const State& s, ConcurrentNode* p, Action a) {

        // only needs to guarantee a unique index, not cross thread memory synchronization
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

        // pre allocate vector capacity to prevent concurrent push_back reallocation
        n->children.reserve(n->untried_space.count);
        
        return n;
    }
};