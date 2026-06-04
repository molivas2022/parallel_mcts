#include "experiment.hpp"
#include "env.hpp"
#include "common.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <memory>

// Internal helper for dashboard
void print_dashboard(const std::vector<ExperimentResult>& past_results, 
                     int current_config, int total_configs, 
                     int n_size, int matches, 
                     const std::string& test_agent_name, int test_iters, int test_threads,
                     int current_match, int current_turn, 
                     int test_wins, int baseline_wins,
                     double accumulated_time) {
    
    std::cout << "\033[2J\033[H";   
    std::cout << "Dashboard\n\n";

    if (!past_results.empty()) {
        std::cout << "Completed Experiments:\n";
        std::cout << std::left 
                  << std::setw(5)  << "N" 
                  << std::setw(15) << "Agent"
                  << std::setw(10) << "Iters"
                  << std::setw(10) << "Threads"
                  << std::setw(12) << "Winrate" 
                  << std::setw(12) << "MCTS Time(s)" << "\n";
        std::cout << std::string(64, '-') << "\n";
        for (const auto& r : past_results) {
            std::cout << std::left 
                      << std::setw(5)  << r.n_size 
                      << std::setw(15) << r.agent_name
                      << std::setw(10) << r.iters
                      << std::setw(10) << r.num_threads
                      << std::fixed << std::setprecision(1)
                      << std::setw(12) << r.winrate 
                      << std::setprecision(2)
                      << std::setw(12) << r.time_seconds << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Current Experiment: " << current_config << " / " << total_configs << "\n";
    std::cout << "N         : " << n_size << "x" << n_size << "\n";
    std::cout << "Testing   : " << test_agent_name << " (" << test_iters << " iters, " << test_threads << " threads)\n";
    std::cout << "Match     : " << current_match << " / " << matches << "\n";
    std::cout << "Turn      : " << current_turn << "\n";
    std::cout << "Test Wins : " << test_wins << "\n";
    std::cout << "Base Wins : " << baseline_wins << "\n";
    std::cout << "MCTS Time : " << std::fixed << std::setprecision(2) << accumulated_time << " seconds\n";
    std::cout << std::flush;
}

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                int matches, 
                                Agent& baseline_agent, 
                                const std::vector<ExperimentResult>& past_results, 
                                int config_num, int total_configs) {
    
    std::unique_ptr<Agent> test_agent;
    if (config.type == AgentType::Sequential) {
        test_agent = std::make_unique<SequentialAgent>(config.iters);
    } else if (config.type == AgentType::LeafParallel) {
        test_agent = std::make_unique<LeafParallelAgent>(config.iters, config.num_threads);
    }

    int test_wins = 0;
    int baseline_wins = 0;
    long long total_turns = 0;
    double mcts_computation_time = 0.0;

    for (int game = 1; game <= matches; ++game) {
        State state = create_initial_state();
        bool test_is_p1 = (game % 2 != 0); 
        int game_turns = 0;
        
        while (state.winner == Player::None) {
            ActionSpace space = get_actions(state);
            if (space.count == 0) break; 

            // Print UI
            print_dashboard(past_results, config_num, total_configs, N, matches, 
                            test_agent->get_name(), test_agent->get_iters(), test_agent->get_num_threads(),
                            game, game_turns + 1, test_wins, baseline_wins, mcts_computation_time);

            Action action;
            
            // Timer start
            auto start_time = std::chrono::high_resolution_clock::now();
            
            if (state.turn == Player::First) {
                action = test_is_p1 ? test_agent->next_action(state) : baseline_agent.next_action(state);
            } else {
                action = test_is_p1 ? baseline_agent.next_action(state) : test_agent->next_action(state);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            // Timer end
            
            std::chrono::duration<double> duration = end_time - start_time;
            mcts_computation_time += duration.count();

            next_state(state, action);
            game_turns++;
        }

        total_turns += game_turns;

        if (state.winner == Player::First) {
            if (test_is_p1) test_wins++; else baseline_wins++;
        } else if (state.winner == Player::Second) {
            if (test_is_p1) baseline_wins++; else test_wins++;
        }
    }

    ExperimentResult res;
    res.n_size = N;
    res.matches = matches;
    res.agent_name = test_agent->get_name();
    res.iters = test_agent->get_iters();
    res.num_threads = test_agent->get_num_threads();
    res.winrate = (static_cast<double>(test_wins) / matches) * 100.0;
    res.avg_turns = static_cast<double>(total_turns) / matches;
    res.time_seconds = mcts_computation_time;

    return res;
}

void save_to_csv(const ExperimentResult& r, bool is_first) {
    std::ofstream file("results.csv", is_first ? std::ios::trunc : std::ios::app);
    if (file.is_open()) {
        if (is_first) {
            file << "N,Matches,Agent,Iters,Threads,Winrate,AvgTurns,MCTS_Time_Seconds\n";
        }
        file << r.n_size << "," << r.matches << "," 
             << r.agent_name << "," << r.iters << "," << r.num_threads << ","
             << std::fixed << std::setprecision(1) << r.winrate << ","
             << r.avg_turns << "," << std::setprecision(2) << r.time_seconds << "\n";
        file.close();
    }
}

void print_final_summary(const std::vector<ExperimentResult>& all_results) {
    std::cout << "\033[2J\033[H";
    std::cout << "All experiments completed!\n\n";

    std::cout << std::left 
              << std::setw(5)  << "N" 
              << std::setw(15) << "Agent"
              << std::setw(10) << "Iters"
              << std::setw(10) << "Threads"
              << std::setw(12) << "Winrate" 
              << std::setw(12) << "Avg Turns" 
              << std::setw(12) << "MCTS Time(s)" << "\n";
    std::cout << std::string(76, '-') << "\n";

    for (const auto& r : all_results) {
        std::cout << std::left 
                  << std::setw(5)  << r.n_size 
                  << std::setw(15) << r.agent_name
                  << std::setw(10) << r.iters
                  << std::setw(10) << r.num_threads
                  << std::fixed << std::setprecision(1)
                  << std::setw(12) << r.winrate 
                  << std::setw(12) << r.avg_turns 
                  << std::setprecision(2)
                  << std::setw(12) << r.time_seconds << "\n";
    }
}