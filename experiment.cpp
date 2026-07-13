#include "experiment.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <memory>

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                const std::vector<State>& dataset, 
                                const std::vector<std::array<double, u_SIZE>>& oracle_cache,
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
        
        // Instantly score the agent's decision against the Oracle cache!
        eval.oracle_score = oracle_cache[i][action.move_idx];
        
        res.evals.push_back(eval);
    }
    std::cout << "] Done.\n";

    return res;
}

void save_raw_csv(const ExperimentResult& r, bool is_first) {
    std::ofstream file("results_raw.csv", is_first ? std::ios::trunc : std::ios::app);
    if (file.is_open()) {
        if (is_first) file << "Config_ID,State_ID,Agent,Sims,Threads,Chosen_Move_Idx,Oracle_Score,Time_Ms\n";
        for (const auto& eval : r.evals) {
            file << r.config_id << "," << eval.state_id << "," << r.agent_name << ","
                 << r.simulations << "," << r.num_threads << "," << eval.chosen_move_idx << ","
                 << std::fixed << std::setprecision(4) << eval.oracle_score << "," 
                 << eval.execution_time_ms << "\n";
        }
        file.close();
    }
}

// Generates the final aggregated data
void save_summary_csv(const std::vector<ExperimentResult>& all_results) {
    std::ofstream file("evaluated_summary.csv", std::ios::trunc);
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
        file.close();
    }
}