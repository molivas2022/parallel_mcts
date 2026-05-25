#pragma once

#include "common.hpp"
#include "dsu.hpp"

#include <array>    // to allocate memory in the stack and not the heap

/* Model */

struct State {
    Player turn;
    Player winner;
    std::array<Player, BOARD_SIZE> board;
    DSU dsu; 
};

struct Action {
    u move_idx;
};

struct ActionSpace {
    std::array<Action, N * N> actions;  // the stack requires compile time alloc
    u count = 0;
};

State create_initial_state();
ActionSpace get_actions(const State& state);
State next_state(const State& state, Action action);