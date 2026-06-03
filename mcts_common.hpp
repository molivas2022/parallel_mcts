#pragma once

#include "common.hpp"
#include "env.hpp"

#include <vector>
#include <functional>

/* Auxiliar structures */

struct Node {
    State state;
    Node* parent;
    Action action;

    std::vector<Node*> children;
    int visits;
    double wins; 
    ActionSpace untried_space;

    Node() = default; 
};

/* Memory Pool */

struct NodePool {
    std::vector<Node> pool;
    int cursor;

    NodePool(size_t capacity);
    
    void reset();
    Node* allocate(const State& s, Node* p, Action a);
};