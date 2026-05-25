#include "env.hpp"

State create_initial_state() {
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

ActionSpace get_actions(const State& state) {
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

void next_state(State& state, Action action) {
    Player current = state.turn;
    
    state.board[action.move_idx] = current;

    for (u offset : NEIGHBOR_OFFSETS) {
        u neighbor = static_cast<u>(action.move_idx + offset);
        if (state.board[neighbor] == current) {
            state.dsu.unite(action.move_idx, neighbor);
        }
    }
    
    if (current == Player::First) {
        if (state.dsu.find(0 * PADDED_N + 1) == state.dsu.find((N + 1) * PADDED_N + 1)) 
            state.winner = Player::First;
    } else {
        if (state.dsu.find(1 * PADDED_N + 0) == state.dsu.find(1 * PADDED_N + (N + 1))) 
            state.winner = Player::Second;
    }

    state.turn = (state.turn == Player::First) ? Player::Second : Player::First;
}