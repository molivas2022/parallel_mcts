#pragma once

#include <array>
#include "common.hpp"
#include "dsu.hpp"

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
    std::array<Action, N * N> actions;
    u count = 0;
};

State create_initial_state();
ActionSpace get_actions(const State& state);
void next_state(State& state, Action action);