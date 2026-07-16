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
    
    // the swap rule
    if (space.count == N * N - 1) {
        space.actions[space.count++] = Action{SWAP_MOVE};
    }
    
    return space;
}

void next_state(State& state, Action action) {
    // handle the swap move
    if (action.move_idx == SWAP_MOVE) {
        u played_r = 0, played_c = 0;
        
        for (u r = 1; r <= N; ++r) {
            for (u c = 1; c <= N; ++c) {
                if (state.board[r * PADDED_N + c] != Player::None) {
                    played_r = r;
                    played_c = c;
                    break;
                }
            }
            if (played_r != 0) break;
        }
        
        state = create_initial_state();
        
        u mirrored_idx = played_c * PADDED_N + played_r;
        state.board[mirrored_idx] = Player::Second;
        
        // update the dsu connections for the newly placed stone
        for (u offset : NEIGHBOR_OFFSETS) {
            u neighbor = static_cast<u>(mirrored_idx + offset);
            if (state.board[neighbor] == Player::Second) {
                state.dsu.unite(mirrored_idx, neighbor);
            }
        }
        
        state.turn = Player::First;
        return;
    }

    // standard move logic
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