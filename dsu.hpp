#pragma once

#include "common.hpp"

#include <array>

/* Auxiliar structure: Disjoint Set Union */

struct DSU {
    std::array<u, BOARD_SIZE> parent;

    void init();
    u find(u i);
    
    // TO DO: its possible to implement rank optimization, but it has a large cost in state overhead (the entire board)
    void unite(u i, u j);
};