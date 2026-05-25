#include "common.hpp"
#include "env.hpp"
#include "mcts.hpp"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>

/* Auxiliar structures and functions */

struct ExperimentResult {
    // parameters
    int n_size;
    int matches;
    int iters_A;
    int iters_B;
    // results
    double winrate_A;
    double winrate_B;
    double avg_turns;
    double time_seconds;
};

// Renders in the console the current progress
void print_dashboard(const std::vector<ExperimentResult>& past_results, 
                     int current_config, int total_configs, 
                     int n_size, int matches, int iters_A, int iters_B,
                     int current_match, int current_turn, 
                     int a_wins, int b_wins,
                     std::chrono::duration<double> elapsed) {
    
    std::cout << "\033[2J\033[H";   // clears screen and resets cursor
    
    std::cout << "Dashboard\n\n";

    // completed experiments
    if (!past_results.empty()) {
        std::cout << "Completed Experiments:\n";
        std::cout << std::left 
                  << std::setw(5)  << "N" 
                  << std::setw(10) << "Iters A" 
                  << std::setw(10) << "Iters B" 
                  << std::setw(10) << "Winrate A" 
                  << std::setw(10) << "Winrate B" 
                  << std::setw(10) << "Time" << "\n";
        std::cout << std::string(55, '-') << "\n";
        for (const auto& r : past_results) {
            std::cout << std::left 
                      << std::setw(5)  << r.n_size 
                      << std::setw(10) << r.iters_A 
                      << std::setw(10) << r.iters_B 
                      << std::fixed << std::setprecision(1)
                      << std::setw(10) << r.winrate_A 
                      << std::setw(10) << r.winrate_B 
                      << std::setprecision(2)
                      << std::setw(10) << r.time_seconds << "\n";
        }
        std::cout << "\n";
    }

    // current experiment
    std::cout << "Current Experiment: " << current_config << " / " << total_configs << "\n";
    std::cout << "N         : " << n_size << "x" << n_size << "\n";
    std::cout << "Iters A   : " << iters_A << " iterations\n";
    std::cout << "Iters B   : " << iters_B << " iterations\n";
    std::cout << "Match     : " << current_match << " / " << matches << "\n";
    std::cout << "Turn      : " << current_turn << "\n";
    std::cout << "Wins A    : " << a_wins << "\n";
    std::cout << "Wins B    : " << b_wins << "\n";
    std::cout << "Time      : " << std::fixed << std::setprecision(1) << elapsed.count() << " seconds\n";
    
    std::cout << std::flush;
}

// Single experiment
ExperimentResult run_experiment(int matches, int iters_A, int iters_B,
                                const std::vector<ExperimentResult>& past_results,
                                int config_num, int total_configs) {
    int a_wins = 0;
    int b_wins = 0;
    long long total_turns = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int game = 1; game <= matches; ++game) {
        State state = create_initial_state();
        bool a_is_p1 = (game % 2 != 0); 
        int game_turns = 0;
        
        while (state.winner == Player::None) {
            ActionSpace space = get_actions(state);
            if (space.count == 0) break; 

            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            print_dashboard(past_results, config_num, total_configs, N, matches, iters_A, iters_B, 
                            game, game_turns + 1, a_wins, b_wins, elapsed);

            Action action;
            if (state.turn == Player::First) {
                action = get_mcts_action(state, a_is_p1 ? iters_A : iters_B);
            } else {
                action = get_mcts_action(state, a_is_p1 ? iters_B : iters_A);
            }

            // Apply the chosen move directly to the main game state
            next_state(state, action);
            game_turns++;
        }

        total_turns += game_turns;

        if (state.winner == Player::First) {
            if (a_is_p1) a_wins++; else b_wins++;
        } else if (state.winner == Player::Second) {
            if (a_is_p1) b_wins++; else a_wins++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    ExperimentResult res;
    res.n_size = N;
    res.matches = matches;
    res.iters_A = iters_A;
    res.iters_B = iters_B;
    res.winrate_A = (static_cast<double>(a_wins) / matches) * 100.0;
    res.winrate_B = (static_cast<double>(b_wins) / matches) * 100.0;
    res.avg_turns = static_cast<double>(total_turns) / matches;
    res.time_seconds = elapsed.count();

    return res;
}

int main() {
    std::vector<ExperimentResult> all_results;
    
    // std::vector<std::pair<int, int>> configs = {
    //     {10000, 20000},
    //     {20000, 40000},
    //     {30000, 60000}
    // };
    std::vector<std::pair<int, int>> configs = {
        {1000, 2000},
        {2000, 4000},
        {3000, 6000}
    };

    // int matches = 30;
    int matches = 1;
    int total_experiments = configs.size();
    int current_exp = 1;

    // helper lambda to save a single result to the csv
    auto save_to_csv = [](const ExperimentResult& r, bool is_first) {
        std::ofstream file("results.csv", is_first ? std::ios::trunc : std::ios::app);
        if (file.is_open()) {
            if (is_first) {
                file << "N,Matches,Iters_A,Iters_B,Winrate_A,Winrate_B,AvgTurns,Time_Seconds\n";
            }
            file << r.n_size << "," << r.matches << "," << r.iters_A << "," << r.iters_B << ","
                 << std::fixed << std::setprecision(1) << r.winrate_A << "," << r.winrate_B << ","
                 << r.avg_turns << "," << std::setprecision(2) << r.time_seconds << "\n";
            file.close();
        }
    };

    bool first_save = true;

    for (const auto& config : configs) {
        auto res = run_experiment(matches, config.first, config.second, all_results, current_exp++, total_experiments);
        all_results.push_back(res);
        save_to_csv(res, first_save);
        first_save = false;
    }

    // final output
    std::cout << "\033[2J\033[H";
    std::cout << "All experiments completed!\n\n";

    std::cout << std::left 
              << std::setw(5)  << "N" 
              << std::setw(10) << "Matches" 
              << std::setw(10) << "Iters A" 
              << std::setw(10) << "Iters B" 
              << std::setw(10) << "Winrate A" 
              << std::setw(10) << "Winrate B" 
              << std::setw(12) << "Avg Turns" 
              << std::setw(10) << "Time" << "\n";
    std::cout << std::string(75, '-') << "\n";

    for (const auto& r : all_results) {
        std::cout << std::left 
                  << std::setw(5)  << r.n_size 
                  << std::setw(10) << r.matches 
                  << std::setw(10) << r.iters_A 
                  << std::setw(10) << r.iters_B 
                  << std::fixed << std::setprecision(1)
                  << std::setw(10) << r.winrate_A 
                  << std::setw(10) << r.winrate_B 
                  << std::setw(12) << r.avg_turns 
                  << std::setprecision(2)
                  << std::setw(10) << r.time_seconds << "\n";
    }

    return 0;
}