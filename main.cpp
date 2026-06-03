#include "common.hpp"
#include "env.hpp"
#include "mcts_common.hpp"
#include "agent.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <iomanip>

struct ExperimentResult {
    int n_size;
    int matches;
    std::string name_A;
    std::string name_B;
    int iters_A;
    int iters_B;
    double winrate_A;
    double winrate_B;
    double avg_turns;
    double time_seconds;
};

void print_dashboard(const std::vector<ExperimentResult>& past_results, 
                     int current_config, int total_configs, 
                     int n_size, int matches, 
                     std::string name_A, int iters_A, 
                     std::string name_B, int iters_B,
                     int current_match, int current_turn, 
                     int a_wins, int b_wins,
                     std::chrono::duration<double> elapsed) {
    
    std::cout << "\033[2J\033[H";   
    std::cout << "Dashboard\n\n";

    if (!past_results.empty()) {
        std::cout << "Completed Experiments:\n";
        std::cout << std::left 
                  << std::setw(5)  << "N" 
                  << std::setw(15) << "Agent A"
                  << std::setw(15) << "Agent B"
                  << std::setw(12) << "Winrate A" 
                  << std::setw(12) << "Winrate B" 
                  << std::setw(10) << "Time" << "\n";
        std::cout << std::string(75, '-') << "\n";
        for (const auto& r : past_results) {
            std::cout << std::left 
                      << std::setw(5)  << r.n_size 
                      << std::setw(15) << r.name_A
                      << std::setw(15) << r.name_B
                      << std::fixed << std::setprecision(1)
                      << std::setw(12) << r.winrate_A 
                      << std::setw(12) << r.winrate_B 
                      << std::setprecision(2)
                      << std::setw(10) << r.time_seconds << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Current Experiment: " << current_config << " / " << total_configs << "\n";
    std::cout << "N         : " << n_size << "x" << n_size << "\n";
    std::cout << "Agent A   : " << name_A << " (" << iters_A << " iters)\n";
    std::cout << "Agent B   : " << name_B << " (" << iters_B << " iters)\n";
    std::cout << "Match     : " << current_match << " / " << matches << "\n";
    std::cout << "Turn      : " << current_turn << "\n";
    std::cout << "Wins A    : " << a_wins << "\n";
    std::cout << "Wins B    : " << b_wins << "\n";
    std::cout << "Time      : " << std::fixed << std::setprecision(1) << elapsed.count() << " seconds\n";
    std::cout << std::flush;
}

ExperimentResult run_experiment(int matches, 
                                Agent& agent_A, Agent& agent_B,
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
            print_dashboard(past_results, config_num, total_configs, N, matches, 
                            agent_A.get_name(), agent_A.get_iters(), 
                            agent_B.get_name(), agent_B.get_iters(), 
                            game, game_turns + 1, a_wins, b_wins, elapsed);

            Action action;
            if (state.turn == Player::First) {
                action = a_is_p1 ? agent_A.next_action(state) : agent_B.next_action(state);
            } else {
                action = a_is_p1 ? agent_B.next_action(state) : agent_A.next_action(state);
            }

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
    res.name_A = agent_A.get_name();
    res.name_B = agent_B.get_name();
    res.iters_A = agent_A.get_iters();
    res.iters_B = agent_B.get_iters();
    res.winrate_A = (static_cast<double>(a_wins) / matches) * 100.0;
    res.winrate_B = (static_cast<double>(b_wins) / matches) * 100.0;
    res.avg_turns = static_cast<double>(total_turns) / matches;
    res.time_seconds = elapsed.count();

    return res;
}

int main() {
    std::vector<ExperimentResult> all_results;
    
    int matches = 1;
    int total_experiments = 2;

    auto save_to_csv = [](const ExperimentResult& r, bool is_first) {
        std::ofstream file("results.csv", is_first ? std::ios::trunc : std::ios::app);
        if (file.is_open()) {
            if (is_first) {
                file << "N,Matches,Agent_A,Iters_A,Agent_B,Iters_B,Winrate_A,Winrate_B,AvgTurns,Time_Seconds\n";
            }
            file << r.n_size << "," << r.matches << "," 
                 << r.name_A << "," << r.iters_A << "," 
                 << r.name_B << "," << r.iters_B << ","
                 << std::fixed << std::setprecision(1) << r.winrate_A << "," << r.winrate_B << ","
                 << r.avg_turns << "," << std::setprecision(2) << r.time_seconds << "\n";
            file.close();
        }
    };

    bool first_save = true;

    // Instantiate the agents
    SequentialAgent dumb_agent(10);
    SequentialAgent seq_agent(10000);
    LeafParallelAgent leaf_agent(2500, 4);

    // Experiment 1: Sequential
    auto res1 = run_experiment(matches, seq_agent, dumb_agent, all_results, 1, total_experiments);
    all_results.push_back(res1);
    save_to_csv(res1, first_save);
    first_save = false;

    // Experiment 2: Leaf
    auto res2 = run_experiment(matches, leaf_agent, dumb_agent, all_results, 2, total_experiments);
    all_results.push_back(res2);
    save_to_csv(res2, first_save);

    // Final output
    std::cout << "\033[2J\033[H";
    std::cout << "All experiments completed!\n\n";

    std::cout << std::left 
              << std::setw(5)  << "N" 
              << std::setw(15) << "Agent A"
              << std::setw(15) << "Agent B"
              << std::setw(12) << "Winrate A" 
              << std::setw(12) << "Winrate B" 
              << std::setw(12) << "Avg Turns" 
              << std::setw(10) << "Time" << "\n";
    std::cout << std::string(85, '-') << "\n";

    for (const auto& r : all_results) {
        std::cout << std::left 
                  << std::setw(5)  << r.n_size 
                  << std::setw(15) << r.name_A
                  << std::setw(15) << r.name_B
                  << std::fixed << std::setprecision(1)
                  << std::setw(12) << r.winrate_A 
                  << std::setw(12) << r.winrate_B 
                  << std::setw(12) << r.avg_turns 
                  << std::setprecision(2)
                  << std::setw(10) << r.time_seconds << "\n";
    }

    return 0;
}