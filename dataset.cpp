#include "dataset.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

void generate_dataset(int num_states, int moves_per_game, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Failed to open " << filename << " for writing.\n";
        exit(1);
    }

    // header: turn, and then the board cells
    file << "Turn";
    for (int i = 0; i < BOARD_SIZE; ++i) {
        file << ",Cell_" << i;
    }
    file << "\n";

    std::mt19937 eng(42); 

    int generated = 0;
    while (generated < num_states) {
        State state = create_initial_state();
        
        // mid-game
        for (int m = 0; m < moves_per_game; ++m) {
            if (state.winner != Player::None) break;
            
            ActionSpace space = get_actions(state);
            if (space.count == 0) break;
            
            std::uniform_int_distribution<int> dist(0, space.count - 1);
            next_state(state, space.actions[dist(eng)]);
        }
        
        // random play accidentally triggered a win
        if (state.winner != Player::None) continue;

        file << static_cast<int>(state.turn);
        
        for (Player cell : state.board) {
            file << "," << static_cast<int>(cell);
        }
        file << "\n";
        
        generated++;
    }
    
    file.close();
    std::cout << "Dataset Generation Complete: " << num_states << " states saved to " << filename << "\n";
}

std::vector<State> load_dataset(const std::string& filename) {
    std::vector<State> dataset;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Failed to open " << filename << " for reading.\n";
        exit(1);
    }

    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string cell;
        
        State s;
        s.winner = Player::None;
        
        std::getline(ss, cell, ',');
        s.turn = static_cast<Player>(std::stoi(cell));
        
        int i = 0;
        while (std::getline(ss, cell, ',') && i < BOARD_SIZE) {
            s.board[i++] = static_cast<Player>(std::stoi(cell));
        }
        
        // safe dsu reconstruction
        s.dsu.init();
        
        // Explicitly unite the virtual padding edges
        for (u r = 1; r <= N; ++r) {
            s.dsu.unite(0 * PADDED_N + 1, 0 * PADDED_N + r);
            s.dsu.unite((N + 1) * PADDED_N + 1, (N + 1) * PADDED_N + r);
            
            s.dsu.unite(1 * PADDED_N + 0, r * PADDED_N + 0);
            s.dsu.unite(1 * PADDED_N + (N + 1), r * PADDED_N + (N + 1));
        }

        // only apply neighbor offsets from the inner playable area
        for (u r = 1; r <= N; ++r) {
            for (u c = 1; c <= N; ++c) {
                u idx = r * PADDED_N + c;
                if (s.board[idx] != Player::None) {
                    for (u offset : NEIGHBOR_OFFSETS) {
                        u neighbor = idx + offset;
                        if (s.board[neighbor] == s.board[idx]) {
                            s.dsu.unite(idx, neighbor);
                        }
                    }
                }
            }
        }
        
        dataset.push_back(s);
    }
    
    file.close();
    return dataset;
}