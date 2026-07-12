#include "experiment.hpp"
#include "agent.hpp"

#include <vector>
#include <chrono>
#include <iostream>

int main() {
    int matches = 50;
    int sims = 5000;

    // int matches = 1;
    // int sims = 100;

    SequentialAgent baseline_agent(sims); 

    std::vector<ExperimentConfig> experiments;

    std::array<int, 3> all_sims_mults = {1, 2, 4};

    std::array<AgentType, 5> all_agent_types = {
        AgentType::LeafParallel,
        AgentType::RootParallel,
        AgentType::LockFree,
        AgentType::VirtualLoss,
        AgentType::WuUct
    };

    std::array<int, 3> all_num_threads = {2, 4, 8};

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

    // Start the clock
    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < experiments.size(); ++i) {
        auto res = run_experiment(experiments[i], matches, baseline_agent, all_results, i + 1, experiments.size());
        all_results.push_back(res);
        
        // Append raw match data as soon as the experiment finishes
        save_raw_csv(res, first_raw_save);
        first_raw_save = false;
    }

    // Stop the clock
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    // Generate the calculated summary table
    save_summary_csv(all_results);
    
    print_final_summary(all_results);

    // Print the total time
    std::cout << "\nTotal time elapsed for all experiments: " 
              << elapsed_seconds.count() << " seconds\n";

    return 0;
}