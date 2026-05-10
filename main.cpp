#include <cstdint>
#include <array>

/* Constants and aliases */

using u = std::uint8_t;

constexpr u N = 3; 

/* Model */

enum class Player : u {
    None,
    First,
    Second
};

struct State {
    Player turn;
    std::array<Player, N * N> board;
};

struct Action {
    Player turn;
    u row;
    u col;
};

struct ActionSpace {
    std::array<Action, N * N> actions;
    u count = 0;
};

/* Utils */

constexpr u flat_to_row(u pos) {
    return pos / N;
}

constexpr u flat_to_col(u pos) {
    return pos % N;
}

constexpr u flatten(u row, u col) {
    return row * N + col;
}

/* Main loop functions */

ActionSpace get_actions(const State& state) {
    ActionSpace space;
    
    for (u i = 0; i < N * N; i++) {
        if (state.board[i] == Player::None) {
            space.actions[space.count] = {state.turn, flat_to_row(i), flat_to_col(i)};
            space.count++;
        }
    }
    
    return space;
}

State next_state(const State& state, Action action) {
    State new_state = state;
    
    new_state.turn = (state.turn == Player::First) ? Player::Second : Player::First;
    
    new_state.board[flatten(action.row, action.col)] = state.turn;
    
    return new_state;
}