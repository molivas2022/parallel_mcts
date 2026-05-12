/*
Used representation: pointy-top
Exanmple of 3x3 grid:
     / \
    |0,0|
   / \ / \
  |1,0|0,1|
 / \ / \ / \
|2,0|1,1|0,2|
 \ / \ / \ /
  |2,1|1,2|
   \ / \ /
    |2,2|
     \ /
*/

#include <iostream>
#include <cstdint>
#include <array>
#include <random>
#include <thread>
#include <chrono>
#include <cmath>

/* Constants */

using u = std::uint8_t;     // max board size: N = 13

constexpr u N = 5; 
constexpr u PADDED_N = N + 2;
constexpr u BOARD_SIZE = PADDED_N * PADDED_N;

constexpr std::array<u, 6> NEIGHBOR_OFFSETS = {
    static_cast<u>(-1),             // up left      (0, -1)
    static_cast<u>(-PADDED_N),      // up right     (-1, 0)
    PADDED_N - 1,                   // left         (1, -1)
    static_cast<u>(-PADDED_N + 1),  // right        (-1, 1)
    PADDED_N,                       // down left    (1, 0)
    1                               // down right   (0, 1)
};

/* Auxiliar structures */

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

    void unite(u i, u j) {
        u root_i = find(i);
        u root_j = find(j);
        if (root_i != root_j) parent[root_i] = root_j;
    }
};

/* Model */

enum class Player : u {
    None,
    First,  // top left to bottom-right
    Second  // top right to bottom left
};

struct State {
    Player turn;
    Player winner;
    std::array<Player, BOARD_SIZE> board;
    DSU dsu; 
};

struct ActionSpace {
    std::array<u, N * N> actions;
    u count = 0;
};

/* Logic */

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
                space.actions[space.count++] = i;
            }
        }
    }
    return space;
}

State next_state(const State& state, u move_index) {
    State next = state;
    Player current = next.turn;
    
    next.board[move_index] = current;

    for (u offset : NEIGHBOR_OFFSETS) {
        u neighbor = static_cast<u>(move_index + offset);
        if (next.board[neighbor] == current) {
            next.dsu.unite(move_index, neighbor);
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

/* Visualizer */

void print_board(const State& state, int turn_count) {
    std::cout << "\033[2J\033[H";   // clear screen and reset cursor
    
    std::cout << "Hex\n";
    std::cout << "Turn: " << turn_count << "\n";
    std::cout << "Player 1: X \tPlayer 2: O\n";

    int max_y = (PADDED_N - 1) * 2;

    for (int y = 0; y <= max_y; ++y) {
        
        // leading spaces to center the diamond
        int leading_spaces = std::abs((int)PADDED_N - 1 - y);
        for (int s = 0; s < leading_spaces; ++s) {
            std::cout << " ";
        }

        // determine the starting col and row for this diagonal level
        // we start from the leftmost piece, which means maximizing 'col'
        int c = std::min(y, (int)PADDED_N - 1);
        int r = y - c;

        // traverse the diagonal down-rightwards across the screen
        while (c >= 0 && r < PADDED_N) {
            u i = static_cast<u>(r * PADDED_N + c);
            
            if (state.board[i] == Player::First) std::cout << "X ";
            else if (state.board[i] == Player::Second) std::cout << "O ";
            else std::cout << ". ";

            // move to the next piece on the same horizontal screen level:
            // this means stepping right: decreasing col, increasing row
            c--;
            r++;
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

/* Simulation */

int main() {
    State state = create_initial_state();
    
    std::random_device rd;
    std::mt19937 engine(rd());

    int turn_count = 0;

    while (state.winner == Player::None) {
        
        print_board(state, turn_count);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        ActionSpace space = get_actions(state);
        if (space.count == 0) break; 

        std::uniform_int_distribution<int> dist(0, space.count - 1);
        u move = space.actions[dist(engine)];

        state = next_state(state, move);
        turn_count++;
    }

    print_board(state, turn_count);
    
    if (state.winner == Player::First) std::cout << "Winner: Player 1\n\n";
    else if (state.winner == Player::Second) std::cout << "Winner: Player 2\n\n";
    
    return 0;
}