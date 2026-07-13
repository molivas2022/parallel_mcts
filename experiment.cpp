/*
 * Module: Experiment Engine
 *
 * Contains the execution loops for both the Match and Oracle benchmark pipelines.
 * The Match pipeline isolates test computation time from the baseline agent. 
 * The Oracle pipeline scores moves instantly using a pre-computed cache.
 */

#include "experiment.hpp"
#include "env.hpp"
#include "common.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <numeric>

// ============================================================================
// Internal Helpers
// ============================================================================

static std::unique_ptr<Agent> create_agent(const ExperimentConfig& config) {
    switch (config.type) {
        case AgentType::Sequential: return std::make_unique<SequentialAgent>(config.simulations);
        case AgentType::LeafParallel: return std::make_unique<LeafParallelAgent>(config.simulations, config.num_threads);
        case AgentType::RootParallel: return std::make_unique<RootParallelAgent>(config.simulations, config.num_threads);
        case AgentType::LockFree: return std::make_unique<LockFreeParallelAgent>(config.simulations, config.num_threads);
        case AgentType::VirtualLoss: return std::make_unique<VirtualLossParallelAgent>(config.simulations, config.num_threads);
        case AgentType::WuUct: return std::make_unique<WuUctParallelAgent>(config.simulations, config.num_threads);
    }
    return nullptr;
}

static void print_dashboard(const std::vector<MatchExperimentResult>& past_results, 
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

// ============================================================================
// Match Pipeline
// ============================================================================

MatchExperimentResult run_match_experiment(const ExperimentConfig& config, 
                                           int matches, 
                                           Agent& baseline_agent, 
                                           const std::vector<MatchExperimentResult>& past_results, 
                                           int config_num, int total_configs) {
    
    auto test_agent = create_agent(config);
    MatchExperimentResult res{config_num, N, test_agent->get_name(), test_agent->get_simulations(), test_agent->get_num_threads(), {}};

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

        res.matches.push_back({game, test_is_p1, test_won, game_turns, test_turns, match_test_time, match_base_time});
    }

    return res;
}

void save_match_raw_csv(const MatchExperimentResult& r, bool is_first) {
    std::ofstream file("match_raw.csv", is_first ? std::ios::trunc : std::ios::app);
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
    }
}

void save_match_summary_csv(const std::vector<MatchExperimentResult>& all_results) {
    std::ofstream file("match_summary.csv", std::ios::trunc);
    if (file.is_open()) {
        file << "Config_ID,Grid_Size,Agent,Sims,Threads,Matches,Global_Winrate,P1_Winrate,P2_Winrate,"
             << "Avg_Game_Turns,Total_Test_Time,Avg_Time_Per_Match,Avg_Time_Per_Turn,Avg_Time_Per_Sim_Ms\n";
        
        for (const auto& r : all_results) {
            int total_matches = r.matches.size();
            int p1_matches = 0, p2_matches = 0;
            int global_wins = 0, p1_wins = 0, p2_wins = 0;
            double total_time = 0.0;
            int total_test_turns = 0, total_game_turns = 0;
            
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
            double avg_match_time = total_matches > 0 ? total_time / total_matches : 0;
            double avg_turn_time = total_test_turns > 0 ? total_time / total_test_turns : 0;
            
            long long total_sims_executed = static_cast<long long>(total_test_turns) * r.simulations;
            double time_per_sim_ms = total_sims_executed > 0 ? (total_time / total_sims_executed) * 1000.0 : 0;

            file << r.config_id << "," << r.n_size << "," << r.agent_name << "," << r.simulations << "," << r.num_threads << ","
                 << total_matches << "," << std::fixed << std::setprecision(1) << global_wr << "," << p1_wr << "," << p2_wr << ","
                 << avg_game_turns << "," << std::setprecision(2) << total_time << "," << avg_match_time << "," 
                 << std::setprecision(4) << avg_turn_time << "," << std::setprecision(6) << time_per_sim_ms << "\n";
        }
    }
}

void print_match_final_summary(const std::vector<MatchExperimentResult>& all_results) {
    std::cout << "\033[2J\033[H";
    std::cout << "Match Pipeline completed! Summary saved to match_summary.csv\n\n";
    std::cout << std::left 
              << std::setw(5)  << "N" << std::setw(15) << "Agent" << std::setw(8)  << "Sims"
              << std::setw(8)  << "Thds" << std::setw(9)  << "Win(Gl)" << std::setw(9)  << "Win(P1)" 
              << std::setw(9)  << "Win(P2)" << std::setw(10) << "AvgTrns" << std::setw(11) << "Time/Trn" 
              << std::setw(15) << "Time/Sim(ms)" << "\n";
    std::cout << std::string(99, '-') << "\n";

    for (const auto& r : all_results) {
        int total_matches = r.matches.size();
        int p1_matches = 0, p2_matches = 0, global_wins = 0, p1_wins = 0, p2_wins = 0;
        double total_time = 0.0;
        int total_test_turns = 0, total_game_turns = 0;
        
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
                  << std::setw(5)  << r.n_size << std::setw(15) << r.agent_name << std::setw(8)  << r.simulations
                  << std::setw(8)  << r.num_threads << std::fixed << std::setprecision(1) << std::setw(9)  << global_wr 
                  << std::setw(9)  << p1_wr << std::setw(9)  << p2_wr << std::setw(10) << avg_game_turns
                  << std::setprecision(4) << std::setw(11) << avg_turn_time 
                  << std::setprecision(6) << std::setw(15) << time_per_sim_ms << "\n";
    }
}

// ============================================================================
// Oracle Pipeline
// ============================================================================

OracleExperimentResult run_oracle_experiment(const ExperimentConfig& config, 
                                             const std::vector<State>& dataset, 
                                             const std::vector<std::array<double, u_SIZE>>& oracle_cache,
                                             int config_num, int total_configs) {
    
    auto test_agent = create_agent(config);
    OracleExperimentResult res{config_num, N, test_agent->get_name(), test_agent->get_simulations(), test_agent->get_num_threads(), {}};

    std::cout << "\nRunning Config " << config_num << "/" << total_configs 
              << " | Agent: " << std::left << std::setw(12) << res.agent_name 
              << " | Sims: " << std::setw(6) << res.simulations 
              << " | Threads: " << res.num_threads << "\nProgress: [";
              
    for (size_t i = 0; i < dataset.size(); ++i) {
        if (i % (dataset.size() / 10 + 1) == 0) std::cout << "#" << std::flush;

        auto start_time = std::chrono::high_resolution_clock::now();
        Action action = test_agent->next_action(dataset[i]);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;

        StateEvalResult eval;
        eval.state_id = i;
        eval.chosen_move_idx = action.move_idx;
        eval.execution_time_ms = duration_ms.count();
        eval.oracle_score = oracle_cache[i][action.move_idx];
        
        res.evals.push_back(eval);
    }
    std::cout << "] Done.\n";

    return res;
}

void save_oracle_raw_csv(const OracleExperimentResult& r, bool is_first) {
    std::ofstream file("oracle_raw.csv", is_first ? std::ios::trunc : std::ios::app);
    if (file.is_open()) {
        if (is_first) file << "Config_ID,State_ID,Agent,Sims,Threads,Chosen_Move_Idx,Oracle_Score,Time_Ms\n";
        for (const auto& eval : r.evals) {
            file << r.config_id << "," << eval.state_id << "," << r.agent_name << ","
                 << r.simulations << "," << r.num_threads << "," << eval.chosen_move_idx << ","
                 << std::fixed << std::setprecision(4) << eval.oracle_score << "," 
                 << eval.execution_time_ms << "\n";
        }
    }
}

void save_oracle_summary_csv(const std::vector<OracleExperimentResult>& all_results) {
    std::ofstream file("oracle_summary.csv", std::ios::trunc);
    if (file.is_open()) {
        file << "Config_ID,Agent,Sims,Threads,Avg_Oracle_Score,Avg_Time_Per_Sim_Ms\n";
        for (const auto& r : all_results) {
            double total_score = 0.0;
            double total_time = 0.0;
            for (const auto& e : r.evals) {
                total_score += e.oracle_score;
                total_time += e.execution_time_ms;
            }
            double avg_score = total_score / r.evals.size();
            double avg_time_ms = total_time / r.evals.size();
            
            file << r.config_id << "," << r.agent_name << "," << r.simulations << "," 
                 << r.num_threads << "," << std::fixed << std::setprecision(4) 
                 << avg_score << "," << avg_time_ms << "\n";
        }
    }
}