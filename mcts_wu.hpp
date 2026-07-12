/*
 * Module: WU-UCT Data Structures
 *
 * This file defines isolated data structures designed exclusively for the 
 * OpenMP Task-Based Pipeline architecture (WU-UCT).
 *
 * Because a single Master thread exclusively performs Selection and Expansion,
 * the tree topology (children vectors, untried spaces) is structurally safe 
 * and requires ZERO locks. However, because OpenMP worker tasks asynchronously 
 * perform Backpropagation, the node statistics (visits, wins, unobserved) 
 * must be atomic to prevent data races.
 */

#pragma once

#include "common.hpp"
#include "env.hpp"

#include <vector>
#include <atomic>
#include <iostream>

struct WuNode {
    State state;
    WuNode* parent;
    Action action;

    std::vector<WuNode*> children;
    
    // Statistics updated asynchronously by OpenMP tasks
    std::atomic<int> visits;
    std::atomic<double> wins; 
    
    // Unobserved samples tracker (on-going incomplete simulation queries)
    std::atomic<int> unobserved; 
    
    ActionSpace untried_space;

    WuNode() {
        visits.store(0, std::memory_order_relaxed);
        wins.store(0.0, std::memory_order_relaxed);
        unobserved.store(0, std::memory_order_relaxed);
    }
    
    // Disable copy/move to prevent atomic state corruption
    WuNode(const WuNode&) = delete;
    WuNode& operator=(const WuNode&) = delete;
};

struct WuNodePool {
    std::vector<WuNode> pool;
    std::atomic<int> cursor;

    WuNodePool(size_t capacity) {
        pool = std::vector<WuNode>(capacity);
        cursor.store(0, std::memory_order_relaxed);
    }
    
    void reset() {
        cursor.store(0, std::memory_order_relaxed); 
    }

    WuNode* allocate(const State& s, WuNode* p, Action a) {
        int idx = cursor.fetch_add(1, std::memory_order_relaxed);
        
        if (static_cast<size_t>(idx) >= pool.size()) {
            std::cerr << "CRITICAL ERROR: WU-MCTS Node pool exhausted!\n";
            exit(1);
        }

        WuNode* n = &pool[idx];
        n->state = s;
        n->parent = p;
        n->action = a;
        n->visits.store(0, std::memory_order_relaxed);
        n->wins.store(0.0, std::memory_order_relaxed);
        n->unobserved.store(0, std::memory_order_relaxed);
        n->children.clear(); 
        n->untried_space = get_actions(s);
        
        // Reserve capacity to prevent vector reallocation
        n->children.reserve(n->untried_space.count);
        
        return n;
    }
};