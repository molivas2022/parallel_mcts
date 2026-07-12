/*
 * Module: Experiment Engine
 *
 * Executes the MCTS agent matches and handles metric collection. The core loop
 * isolates the computation time of the test agent from the baseline agent to
 * provide accurate performance profiling. It also generates both normalized raw 
 * data logs and aggregated summary reports.
 */

#include "experiment.hpp"
#include "env.hpp"
#include "common.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <memory>
#include <numeric>

void print_dashboard(const std::vector<ExperimentResult>& past_results, 
                     int current_config, int total_configs, 
                     int n_size, int matches, 
                     const std::string& test_agent_name, int test_sims, int test_threads,
                     int current_match, int current_turn, 
                     int test_wins, int baseline_wins,
                     double accumulated_test_time) {
    
    std::cout << "\033[2J\033[H";   
    std::cout << "Dashboard\n\n";

    if (!past_results.empty()) {
        std::cout << "Completed Experiments:\n";
        std::cout << std::left 
                  << std::setw(15) << "Agent"
                  << std::setw(10) << "Sims"
                  << std::setw(10) << "Threads"
                  << std::setw(12) << "Matches" 
                  << std::setw(15) << "Test Time(s)" << "\n";
        std::cout << std::string(62, '-') << "\n";
        for (const auto& r : past_results) {
            double total_time = 0.0;
            for (const auto& m : r.matches) total_time += m.test_mcts_time;
            
            std::cout << std::left 
                      << std::setw(15) << r.agent_name
                      << std::setw(10) << r.simulations
                      << std::setw(10) << r.num_threads
                      << std::setw(12) << r.matches.size() 
                      << std::fixed << std::setprecision(2)
                      << std::setw(15) << total_time << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Current Experiment: " << current_config << " / " << total_configs << "\n";
    std::cout << "N         : " << n_size << "x" << n_size << "\n";
    std::cout << "Testing   : " << test_agent_name << " (" << test_sims << " sims, " << test_threads << " threads)\n";
    std::cout << "Match     : " << current_match << " / " << matches << "\n";
    std::cout << "Turn      : " << current_turn << "\n";
    std::cout << "Test Wins : " << test_wins << "\n";
    std::cout << "Base Wins : " << baseline_wins << "\n";
    std::cout << "Test Time : " << std::fixed << std::setprecision(2) << accumulated_test_time << " seconds\n";
    std::cout << std::flush;
}

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                int matches, 
                                Agent& baseline_agent, 
                                const std::vector<ExperimentResult>& past_results, 
                                int config_num, int total_configs) {
    
    std::unique_ptr<Agent> test_agent;
    if (config.type == AgentType::Sequential) {
        test_agent = std::make_unique<SequentialAgent>(config.simulations);
    } else if (config.type == AgentType::LeafParallel) {
        test_agent = std::make_unique<LeafParallelAgent>(config.simulations, config.num_threads);
    } else if (config.type == AgentType::RootParallel) {
        test_agent = std::make_unique<RootParallelAgent>(config.simulations, config.num_threads);
    } else if (config.type == AgentType::LockFree) {
        test_agent = std::make_unique<LockFreeParallelAgent>(config.simulations, config.num_threads);
    } else if (config.type == AgentType::VirtualLoss) {
        test_agent = std::make_unique<VirtualLossParallelAgent>(config.simulations, config.num_threads);
    } else if (config.type == AgentType::WuUct) {
        test_agent = std::make_unique<WuUctParallelAgent>(config.simulations, config.num_threads);
    }

    ExperimentResult res;
    res.config_id = config_num;
    res.n_size = N;
    res.agent_name = test_agent->get_name();
    res.simulations = test_agent->get_simulations();
    res.num_threads = test_agent->get_num_threads();

    int test_wins = 0;
    int baseline_wins = 0;
    double accumulated_test_time = 0.0;

    for (int game = 1; game <= matches; ++game) {
        State state = create_initial_state();
        bool test_is_p1 = (game % 2 != 0); 
        
        int game_turns = 0;
        int test_turns = 0;
        double match_test_time = 0.0;
        double match_base_time = 0.0;
        
        while (state.winner == Player::None) {
            ActionSpace space = get_actions(state);
            if (space.count == 0) break; 

            print_dashboard(past_results, config_num, total_configs, N, matches, 
                            res.agent_name, res.simulations, res.num_threads,
                            game, game_turns + 1, test_wins, baseline_wins, accumulated_test_time);

            Action action;
            bool is_test_turn = (state.turn == Player::First && test_is_p1) || 
                                (state.turn == Player::Second && !test_is_p1);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            if (is_test_turn) {
                action = test_agent->next_action(state);
            } else {
                action = baseline_agent.next_action(state);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            
            if (is_test_turn) {
                match_test_time += duration.count();
                accumulated_test_time += duration.count();
                test_turns++;
            } else {
                match_base_time += duration.count();
            }

            next_state(state, action);
            game_turns++;
        }

        bool test_won = false;
        if (state.winner == Player::First) {
            if (test_is_p1) { test_won = true; test_wins++; } else baseline_wins++;
        } else if (state.winner == Player::Second) {
            if (!test_is_p1) { test_won = true; test_wins++; } else baseline_wins++;
        }

        MatchResult m_res;
        m_res.match_num = game;
        m_res.test_is_p1 = test_is_p1;
        m_res.test_won = test_won;
        m_res.game_turns = game_turns;
        m_res.test_agent_turns = test_turns;
        m_res.test_mcts_time = match_test_time;
        m_res.baseline_mcts_time = match_base_time;
        
        res.matches.push_back(m_res);
    }

    return res;
}

void save_raw_csv(const ExperimentResult& r, bool is_first) {
    std::ofstream file("results_raw.csv", is_first ? std::ios::trunc : std::ios::app);
    if (file.is_open()) {
        if (is_first) {
            file << "Config_ID,Agent,Sims,Threads,Match_Num,Test_Is_P1,Test_Won,Total_Turns,Test_Turns,Test_Time_Sec,Base_Time_Sec\n";
        }
        for (const auto& m : r.matches) {
            file << r.config_id << "," << r.agent_name << "," << r.simulations << "," << r.num_threads << ","
                 << m.match_num << "," << (m.test_is_p1 ? 1 : 0) << "," << (m.test_won ? 1 : 0) << ","
                 << m.game_turns << "," << m.test_agent_turns << ","
                 << std::fixed << std::setprecision(4) << m.test_mcts_time << "," 
                 << std::setprecision(4) << m.baseline_mcts_time << "\n";
        }
        file.close();
    }
}

void save_summary_csv(const std::vector<ExperimentResult>& all_results) {
    std::ofstream file("results_summary.csv", std::ios::trunc);
    if (file.is_open()) {
        // Added Grid_Size and Avg_Game_Turns to the CSV header
        file << "Config_ID,Grid_Size,Agent,Sims,Threads,Matches,Global_Winrate,P1_Winrate,P2_Winrate,"
             << "Avg_Game_Turns,Total_Test_Time,Avg_Time_Per_Match,Avg_Time_Per_Turn,Avg_Time_Per_Sim_Ms\n";
        
        for (const auto& r : all_results) {
            int total_matches = r.matches.size();
            int p1_matches = 0, p2_matches = 0;
            int global_wins = 0, p1_wins = 0, p2_wins = 0;
            double total_time = 0.0;
            int total_test_turns = 0;
            int total_game_turns = 0; // Tracks total turns across all matches
            
            for (const auto& m : r.matches) {
                if (m.test_won) global_wins++;
                if (m.test_is_p1) {
                    p1_matches++;
                    if (m.test_won) p1_wins++;
                } else {
                    p2_matches++;
                    if (m.test_won) p2_wins++;
                }
                total_time += m.test_mcts_time;
                total_test_turns += m.test_agent_turns;
                total_game_turns += m.game_turns;
            }
            
            double global_wr = total_matches > 0 ? (static_cast<double>(global_wins) / total_matches) * 100.0 : 0;
            double p1_wr = p1_matches > 0 ? (static_cast<double>(p1_wins) / p1_matches) * 100.0 : 0;
            double p2_wr = p2_matches > 0 ? (static_cast<double>(p2_wins) / p2_matches) * 100.0 : 0;
            double avg_game_turns = total_matches > 0 ? static_cast<double>(total_game_turns) / total_matches : 0;
            
            double avg_match_time = total_matches > 0 ? total_time / total_matches : 0;
            double avg_turn_time = total_test_turns > 0 ? total_time / total_test_turns : 0;
            
            long long total_sims_executed = static_cast<long long>(total_test_turns) * r.simulations;
            double time_per_sim_ms = total_sims_executed > 0 ? (total_time / total_sims_executed) * 1000.0 : 0;

            file << r.config_id << "," << r.n_size << "," << r.agent_name << "," << r.simulations << "," << r.num_threads << ","
                 << total_matches << "," 
                 << std::fixed << std::setprecision(1) << global_wr << "," << p1_wr << "," << p2_wr << ","
                 << avg_game_turns << ","
                 << std::setprecision(2) << total_time << "," << avg_match_time << "," 
                 << std::setprecision(4) << avg_turn_time << "," 
                 << std::setprecision(6) << time_per_sim_ms << "\n";
        }
        file.close();
    }
}

void print_final_summary(const std::vector<ExperimentResult>& all_results) {
    std::cout << "\033[2J\033[H";
    std::cout << "All experiments completed! Summary saved to results_summary.csv\n\n";

    // Realigned headers to fit the new columns neatly on screen
    std::cout << std::left 
              << std::setw(5)  << "N"
              << std::setw(15) << "Agent"
              << std::setw(8)  << "Sims"
              << std::setw(8)  << "Thds"
              << std::setw(9)  << "Win(Gl)" 
              << std::setw(9)  << "Win(P1)" 
              << std::setw(9)  << "Win(P2)" 
              << std::setw(10) << "AvgTrns" 
              << std::setw(11) << "Time/Trn" 
              << std::setw(15) << "Time/Sim(ms)" << "\n";
    std::cout << std::string(99, '-') << "\n";

    for (const auto& r : all_results) {
        int total_matches = r.matches.size();
        int p1_matches = 0, p2_matches = 0;
        int global_wins = 0, p1_wins = 0, p2_wins = 0;
        double total_time = 0.0;
        int total_test_turns = 0;
        int total_game_turns = 0;
        
        for (const auto& m : r.matches) {
            if (m.test_won) global_wins++;
            if (m.test_is_p1) { p1_matches++; if (m.test_won) p1_wins++; } 
            else { p2_matches++; if (m.test_won) p2_wins++; }
            total_time += m.test_mcts_time;
            total_test_turns += m.test_agent_turns;
            total_game_turns += m.game_turns;
        }
        
        double global_wr = total_matches > 0 ? (static_cast<double>(global_wins) / total_matches) * 100.0 : 0;
        double p1_wr = p1_matches > 0 ? (static_cast<double>(p1_wins) / p1_matches) * 100.0 : 0;
        double p2_wr = p2_matches > 0 ? (static_cast<double>(p2_wins) / p2_matches) * 100.0 : 0;
        double avg_game_turns = total_matches > 0 ? static_cast<double>(total_game_turns) / total_matches : 0;
        double avg_turn_time = total_test_turns > 0 ? total_time / total_test_turns : 0;
        
        long long total_sims_executed = static_cast<long long>(total_test_turns) * r.simulations;
        double time_per_sim_ms = total_sims_executed > 0 ? (total_time / total_sims_executed) * 1000.0 : 0;

        std::cout << std::left 
                  << std::setw(5)  << r.n_size
                  << std::setw(15) << r.agent_name
                  << std::setw(8)  << r.simulations
                  << std::setw(8)  << r.num_threads
                  << std::fixed << std::setprecision(1)
                  << std::setw(9)  << global_wr 
                  << std::setw(9)  << p1_wr 
                  << std::setw(9)  << p2_wr 
                  << std::setw(10) << avg_game_turns
                  << std::setprecision(4)
                  << std::setw(11) << avg_turn_time 
                  << std::setprecision(6)
                  << std::setw(15) << time_per_sim_ms << "\n";
    }
}