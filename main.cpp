/*
 * Module: Main Entry Point
 *
 * Orchestrates the execution of the MCTS benchmark suite. It provides an 
 * interactive prompt to select between the Match Pipeline (full games) and 
 * the Oracle Pipeline (static evaluation). Configuration grids (simulations,
 * threads, agents) are strictly isolated between the two pipeline modes.
 */

#include "experiment.hpp"
#include "agent.hpp"
#include "dataset.hpp"

#include <vector>
#include <chrono>
#include <iostream>
#include <array>
#include <string>

int main(int argc, char* argv[]) {
    // ========================================================================
    // Match Pipeline Configuration
    // ========================================================================
    int match_games = 100;           
    int match_baseline_sims = 5000; // The fixed strength of the opponent
    
    std::vector<int> match_sims_list = {2500, 5000, 10000};
    std::vector<int> match_num_threads = {2, 3, 4};
    std::vector<AgentType> match_agent_types = {
        AgentType::LeafParallel, AgentType::RootParallel,
        AgentType::LockFree, AgentType::VirtualLoss, AgentType::WuUct
    };

    std::vector<ExperimentConfig> match_experiments;
    for (int sims : match_sims_list) {
        match_experiments.push_back({AgentType::Sequential, sims, 1});
        for (AgentType agent_type : match_agent_types) {
            for (int num_threads : match_num_threads) {
                match_experiments.push_back({agent_type, sims, num_threads});
            }
        }
    }

    // ========================================================================
    // Oracle Pipeline Configuration
    // ========================================================================
    int oracle_dataset_size = 500;  
    int oracle_cache_sims = 100000; // Deep search budget for the Oracle truth
    int oracle_repetitions = 5;
    
    std::vector<int> oracle_sims_list = {1000, 2500, 5000};
    std::vector<int> oracle_num_threads = {2, 3, 4};
    std::vector<AgentType> oracle_agent_types = {
        AgentType::LeafParallel, AgentType::RootParallel,
        AgentType::LockFree, AgentType::VirtualLoss, AgentType::WuUct
    };

    std::vector<ExperimentConfig> oracle_experiments;
    for (int sims : oracle_sims_list) {
        oracle_experiments.push_back({AgentType::Sequential, sims, 1});
        for (AgentType agent_type : oracle_agent_types) {
            for (int num_threads : oracle_num_threads) {
                oracle_experiments.push_back({agent_type, sims, num_threads});
            }
        }
    }

    // ========================================================================
    // CLI Parsing / Interactive Menu
    // ========================================================================
    bool run_match = false;
    bool run_oracle = false;

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--match") run_match = true;
        else if (arg == "--oracle") run_oracle = true;
    } else {
        std::cout << "========================================\n";
        std::cout << "        MCTS HEX BENCHMARK SUITE        \n";
        std::cout << "========================================\n";
        std::cout << "1. Run Match Pipeline (Full Games)\n";
        std::cout << "2. Run Oracle Pipeline (Static Eval)\n";
        std::cout << "3. Run Both\n";
        std::cout << "Select mode (1-3): ";
        
        int choice;
        std::cin >> choice;
        run_match = (choice == 1 || choice == 3);
        run_oracle = (choice == 2 || choice == 3);
    }

    auto global_start_time = std::chrono::steady_clock::now();

    // ========================================================================
    // Execute Match Pipeline
    // ========================================================================
    if (run_match) {
        std::cout << "\n[=== STARTING MATCH PIPELINE (N=" << N << ") ===]\n";
        VirtualLossParallelAgent baseline_agent(match_baseline_sims, 4); 
        std::vector<MatchExperimentResult> match_results;
        bool first_raw_save = true;

        for (size_t i = 0; i < match_experiments.size(); ++i) {
            auto res = run_match_experiment(match_experiments[i], match_games, baseline_agent, match_results, i + 1, match_experiments.size());
            match_results.push_back(res);
            
            save_match_raw_csv(res, first_raw_save);
            first_raw_save = false;
        }
        print_match_final_summary(match_results);
    }

    // ========================================================================
    // Execute Oracle Pipeline
    // ========================================================================
    if (run_oracle) {
        std::cout << "\n[=== STARTING ORACLE PIPELINE (N=" << N << ") ===]\n";
        std::string dataset_file = "dataset.csv";
        
        // Uncomment the next line if you need to generate a fresh dataset
        // generate_dataset(oracle_dataset_size, N*N/2, dataset_file);

        std::cout << "Loading dataset from " << dataset_file << "...\n";
        std::vector<State> dataset = load_dataset(dataset_file);
        
        std::cout << "\n>>> Pre-computing " << oracle_cache_sims << " sims Oracle Cache for " 
                  << dataset.size() << " states (This takes a moment)...\n";
        std::vector<std::array<double, u_SIZE>> oracle_cache;
        
        // Oracle is a heavy Virtual Loss search
        VirtualLossParallelAgent oracle(oracle_cache_sims, 4, oracle_cache_sims*2); 
        for (size_t i = 0; i < dataset.size(); ++i) {
            oracle_cache.push_back(oracle.get_action_scores(dataset[i]));
            std::cout << "Oracle mapped state " << i + 1 << "/" << dataset.size() << "\r" << std::flush;
        }
        std::cout << "\nOracle Cache complete! Starting benchmark...\n";

        std::vector<OracleExperimentResult> oracle_results;
        bool first_raw_save = true;

        for (size_t i = 0; i < oracle_experiments.size(); ++i) {
            auto res = run_oracle_experiment(oracle_experiments[i], dataset, oracle_cache, oracle_repetitions, i + 1, oracle_experiments.size());
            oracle_results.push_back(res);
            
            save_oracle_raw_csv(res, first_raw_save);
            first_raw_save = false;
        }
        std::cout << "\nOracle Pipeline completed! Raw data saved to oracle_raw.csv\n";
    }

    auto global_end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = global_end_time - global_start_time;
    std::cout << "\nTotal execution time: " << elapsed_seconds.count() << " seconds.\n";

    return 0;
}