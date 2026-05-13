#pragma once

#include <array>

#include "common.cpp"

/* Auxiliar structure: Disjoint Set Union */

struct DSU {
    std::array<u, BOARD_SIZE> parent;

    void init() {
        for (u i = 0; i < BOARD_SIZE; ++i) parent[i] = i;
    }

    u find(u i) {
        u root = i;
        while (root != parent[root]) root = parent[root];
        u curr = i;
        while (curr != root) {
            u nxt = parent[curr];
            parent[curr] = root;
            curr = nxt;
        }
        return root;
    }

    // NOTE: its possible to implement rank optimization, but it has a large cost in state overhead (the entire board)
    void unite(u i, u j) {
        u root_i = find(i);
        u root_j = find(j);
        if (root_i != root_j) parent[root_i] = root_j;
    }
};
