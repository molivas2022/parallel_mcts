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

    // Header: Turn, and then the 169 board cells
    file << "Turn";
    for (int i = 0; i < BOARD_SIZE; ++i) {
        file << ",Cell_" << i;
    }
    file << "\n";

    // Fixed seed for reproducibility so you test on the exact same dataset every time
    std::mt19937 eng(42); 

    int generated = 0;
    while (generated < num_states) {
        State state = create_initial_state();
        
        // Advance the board into the mid-game
        for (int m = 0; m < moves_per_game; ++m) {
            if (state.winner != Player::None) break;
            
            ActionSpace space = get_actions(state);
            if (space.count == 0) break;
            
            std::uniform_int_distribution<int> dist(0, space.count - 1);
            next_state(state, space.actions[dist(eng)]);
        }
        
        // If random play accidentally triggered a win, discard the board and retry
        if (state.winner != Player::None) continue;

        // Export the turn (1 for First, 2 for Second)
        file << static_cast<int>(state.turn);
        
        // Export the raw padded board array
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
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string cell;
        
        State s;
        s.winner = Player::None;
        
        // Parse Turn
        std::getline(ss, cell, ',');
        s.turn = static_cast<Player>(std::stoi(cell));
        
        // Parse Board
        int i = 0;
        while (std::getline(ss, cell, ',') && i < BOARD_SIZE) {
            s.board[i++] = static_cast<Player>(std::stoi(cell));
        }
        
        // Rebuild DSU Connections
        s.dsu.init();
        for (u idx = 0; idx < BOARD_SIZE; ++idx) {
            if (s.board[idx] != Player::None) {
                for (u offset : NEIGHBOR_OFFSETS) {
                    u neighbor = idx + offset;
                    // Because the board is padded, neighbors will never wrap around dangerously
                    if (neighbor < BOARD_SIZE && s.board[neighbor] == s.board[idx]) {
                        s.dsu.unite(idx, neighbor);
                    }
                }
            }
        }
        
        dataset.push_back(s);
    }
    
    file.close();
    return dataset;
}