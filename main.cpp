#include "experiment.hpp"
#include "agent.hpp"
#include "dataset.hpp"

#include <vector>
#include <chrono>
#include <iostream>
#include <array>

int main() {
    int dataset_size = 100;
    int sims = 10000;
    int oracle_sims = 100000;
    std::array<int, 3> all_sims_mults = {1, 2, 4};
    std::array<int, 3> all_num_threads = {2, 4, 8};
    
    std::string dataset_file = "dataset.csv";
    
    // Uncomment to generate a fresh dataset
    generate_dataset(dataset_size, N*N/2, dataset_file);

    std::cout << "Loading dataset...\n";
    std::vector<State> dataset = load_dataset(dataset_file);
    std::cout << "Loaded " << dataset.size() << " states.\n";

    // ---------------------------------------------------------
    // ORACLE PRE-COMPUTATION
    // ---------------------------------------------------------
    std::cout << "\n>>> Pre-computing 100k Oracle Cache for " << dataset.size() << " states (This takes a moment)...\n";
    std::vector<std::array<double, u_SIZE>> oracle_cache;
    VirtualLossParallelAgent oracle(oracle_sims, 4, 500000); 
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        oracle_cache.push_back(oracle.get_action_scores(dataset[i]));
        std::cout << "Oracle mapped state " << i + 1 << "/" << dataset.size() << "\r" << std::flush;
    }
    std::cout << "\nOracle Cache complete! Starting benchmark...\n";

    // ---------------------------------------------------------
    // EXPERIMENT EXECUTION
    // ---------------------------------------------------------
    std::vector<ExperimentConfig> experiments;
    std::array<AgentType, 5> all_agent_types = {
        AgentType::LeafParallel, AgentType::RootParallel,
        AgentType::LockFree, AgentType::VirtualLoss, AgentType::WuUct
    };

    for (int sims_mult: all_sims_mults) {
        experiments.push_back({AgentType::Sequential, sims * sims_mult, 1});
        for (AgentType agent_type: all_agent_types) {
            for (int num_threads: all_num_threads) {
                experiments.push_back({agent_type, sims * sims_mult, num_threads});
            }
        }
    }

    std::vector<ExperimentResult> all_results;
    bool first_raw_save = true;
    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < experiments.size(); ++i) {
        auto res = run_experiment(experiments[i], dataset, oracle_cache, i + 1, experiments.size());
        all_results.push_back(res);
        save_raw_csv(res, first_raw_save);
        first_raw_save = false;
    }

    // Save the final aggregated summary native to C++
    save_summary_csv(all_results);

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    std::cout << "\nTotal time elapsed for all experiments: " << elapsed_seconds.count() << " seconds\n";
    std::cout << "Data saved to evaluated_summary.csv. Ready for visualization.\n";

    return 0;
}