#include "dsu.hpp"

void DSU::init() {
    for (u i = 0; i < BOARD_SIZE; ++i) parent[i] = i;
}

u DSU::find(u i) {
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

void DSU::unite(u i, u j) {
    u root_i = find(i);
    u root_j = find(j);
    if (root_i != root_j) parent[root_i] = root_j;
}