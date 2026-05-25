#include "mcts.hpp"

#include <iostream>

/* Memory Pool */

NodePool::NodePool(size_t capacity) {
    pool.resize(capacity);
    cursor = 0;
}

void NodePool::reset() {
    cursor = 0; 
}

Node* NodePool::allocate(const State& s, Node* p, Action a) {
    if (cursor >= pool.size()) {
        std::cerr << "CRITICAL ERROR: Node pool exhausted!\n";
        exit(1);
    }

    Node* n = &pool[cursor++];
    
    n->state = s;
    n->parent = p;
    n->action = a;
    n->visits = 0;
    n->wins = 0.0;
    n->children.clear(); 
    n->untried_space = get_actions(s);
    
    return n;
}