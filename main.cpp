/*
 * Module: Main Entry Point
 *
 * Orchestrates the execution of the MCTS benchmark suite. It provides an 
 * interactive prompt to select between the Match Pipeline (full games) and 
 * the Oracle Pipeline (static evaluation). It constructs the experiment grid
 * and dispatches them to the appropriate runners.
 */

#include "experiment.hpp"
#include "agent.hpp"
#include "dataset.hpp"

#include <vector>
#include <chrono>
#include <iostream>
#include <array>
#include <string>

int main() {
    // ========================================================================
    // Global Configuration
    // ========================================================================
    // int match_games = 50;           // Games per config in Match Mode
    // int oracle_dataset_size = 100;  // Number of static states in Oracle Mode
    
    // int base_sims = 5000;
    // int oracle_cache_sims = 100000; // Deep search budget for the Oracle truth
    
    // std::array<int, 3> all_sims_mults = {1, 2, 4};
    // std::array<int, 3> all_num_threads = {2, 4, 8};
    // std::array<AgentType, 5> all_agent_types = {
    //     AgentType::LeafParallel, AgentType::RootParallel,
    //     AgentType::LockFree, AgentType::VirtualLoss, AgentType::WuUct
    // };

    int match_games = 5;           // Games per config in Match Mode
    int oracle_dataset_size = 10;  // Number of static states in Oracle Mode
    
    int base_sims = 500;
    int oracle_cache_sims = 10000; // Deep search budget for the Oracle truth
    
    std::array<int, 2> all_sims_mults = {1, 2};
    std::array<int, 2> all_num_threads = {2, 4};
    std::array<AgentType, 5> all_agent_types = {
        AgentType::LeafParallel, AgentType::RootParallel,
        AgentType::LockFree, AgentType::VirtualLoss, AgentType::WuUct
    };

    // Build the grid of experiments
    std::vector<ExperimentConfig> experiments;
    for (int sims_mult : all_sims_mults) {
        // Baseline Sequential Agent (1 thread)
        experiments.push_back({AgentType::Sequential, base_sims * sims_mult, 1});
        
        // Parallel Agents
        for (AgentType agent_type : all_agent_types) {
            for (int num_threads : all_num_threads) {
                experiments.push_back({agent_type, base_sims * sims_mult, num_threads});
            }
        }
    }

    // ========================================================================
    // Interactive Menu
    // ========================================================================
    std::cout << "========================================\n";
    std::cout << "        MCTS HEX BENCHMARK SUITE        \n";
    std::cout << "========================================\n";
    std::cout << "1. Run Match Pipeline (Full Games)\n";
    std::cout << "2. Run Oracle Pipeline (Static Eval)\n";
    std::cout << "3. Run Both\n";
    std::cout << "Select mode (1-3): ";
    
    int choice;
    std::cin >> choice;
    
    bool run_match = (choice == 1 || choice == 3);
    bool run_oracle = (choice == 2 || choice == 3);

    auto global_start_time = std::chrono::steady_clock::now();

    // ========================================================================
    // Execute Match Pipeline
    // ========================================================================
    if (run_match) {
        std::cout << "\n[=== STARTING MATCH PIPELINE ===]\n";
        SequentialAgent baseline_agent(base_sims); 
        std::vector<MatchExperimentResult> match_results;
        bool first_raw_save = true;

        for (size_t i = 0; i < experiments.size(); ++i) {
            auto res = run_match_experiment(experiments[i], match_games, baseline_agent, match_results, i + 1, experiments.size());
            match_results.push_back(res);
            
            save_match_raw_csv(res, first_raw_save);
            first_raw_save = false;
        }

        save_match_summary_csv(match_results);
        print_match_final_summary(match_results);
    }

    // ========================================================================
    // Execute Oracle Pipeline
    // ========================================================================
    if (run_oracle) {
        std::cout << "\n[=== STARTING ORACLE PIPELINE ===]\n";
        std::string dataset_file = "dataset.csv";
        
        // Uncomment the next line if you need to generate a fresh dataset
        generate_dataset(oracle_dataset_size, N*N/2, dataset_file);

        std::cout << "Loading dataset from " << dataset_file << "...\n";
        std::vector<State> dataset = load_dataset(dataset_file);
        
        std::cout << "\n>>> Pre-computing 100k Oracle Cache for " << dataset.size() << " states (This takes a moment)...\n";
        std::vector<std::array<double, u_SIZE>> oracle_cache;
        
        // Oracle is a heavy Virtual Loss search
        VirtualLossParallelAgent oracle(oracle_cache_sims, 4, 500000); 
        for (size_t i = 0; i < dataset.size(); ++i) {
            oracle_cache.push_back(oracle.get_action_scores(dataset[i]));
            std::cout << "Oracle mapped state " << i + 1 << "/" << dataset.size() << "\r" << std::flush;
        }
        std::cout << "\nOracle Cache complete! Starting benchmark...\n";

        std::vector<OracleExperimentResult> oracle_results;
        bool first_raw_save = true;

        for (size_t i = 0; i < experiments.size(); ++i) {
            auto res = run_oracle_experiment(experiments[i], dataset, oracle_cache, i + 1, experiments.size());
            oracle_results.push_back(res);
            
            save_oracle_raw_csv(res, first_raw_save);
            first_raw_save = false;
        }

        save_oracle_summary_csv(oracle_results);
        std::cout << "\nOracle Pipeline completed! Summary saved to oracle_summary.csv\n";
    }

    auto global_end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = global_end_time - global_start_time;
    std::cout << "\nTotal execution time: " << elapsed_seconds.count() << " seconds.\n";

    return 0;
}