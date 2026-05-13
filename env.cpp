#pragma once

#include <array>    // to allocate memory in the stack and not the heap

#include "common.cpp"
#include "dsu.cpp"

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

static State create_initial_state() {
    State s;
    s.turn = Player::First;
    s.winner = Player::None;
    s.board.fill(Player::None); 
    s.dsu.init();

    for (u i = 1; i <= N; ++i) {
        s.board[0 * PADDED_N + i] = Player::First;
        s.board[(N + 1) * PADDED_N + i] = Player::First;
        s.dsu.unite(0 * PADDED_N + 1, 0 * PADDED_N + i);
        s.dsu.unite((N + 1) * PADDED_N + 1, (N + 1) * PADDED_N + i);

        s.board[i * PADDED_N + 0] = Player::Second;
        s.board[i * PADDED_N + (N + 1)] = Player::Second;
        s.dsu.unite(1 * PADDED_N + 0, i * PADDED_N + 0);
        s.dsu.unite(1 * PADDED_N + (N + 1), i * PADDED_N + (N + 1));
    }
    return s;
}

static ActionSpace get_actions(const State& state) {
    ActionSpace space;
    for (u r = 1; r <= N; ++r) {
        for (u c = 1; c <= N; ++c) {
            u i = r * PADDED_N + c;
            if (state.board[i] == Player::None) {
                space.actions[space.count++] = Action{i};
            }
        }
    }
    return space;
}

static State next_state(const State& state, Action action) {
    State next = state;
    Player current = next.turn;
    
    next.board[action.move_idx] = current;

    for (u offset : NEIGHBOR_OFFSETS) {
        u neighbor = static_cast<u>(action.move_idx + offset);
        if (next.board[neighbor] == current) {
            next.dsu.unite(action.move_idx, neighbor);
        }
    }
    
    if (current == Player::First) {
        if (next.dsu.find(0 * PADDED_N + 1) == next.dsu.find((N + 1) * PADDED_N + 1)) 
            next.winner = Player::First;
    } else {
        if (next.dsu.find(1 * PADDED_N + 0) == next.dsu.find(1 * PADDED_N + (N + 1))) 
            next.winner = Player::Second;
    }

    next.turn = (state.turn == Player::First) ? Player::Second : Player::First;
    return next;
}