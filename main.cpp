#include "experiment.hpp"
#include "agent.hpp"
#include "dataset.hpp"

#include <vector>
#include <chrono>
#include <iostream>

int main() {

    // Parameters

    // int dataset_size = 100;
    // int sims = 5000;
    // std::array<int, 3> all_sims_mults = {1, 2, 4};
    // std::array<int, 3> all_num_threads = {2, 4, 8};

    int dataset_size = 3;
    int sims = 5000;
    std::array<int, 2> all_sims_mults = {1, 2};
    std::array<int, 2> all_num_threads = {2, 4};

    std::string dataset_file = "dataset.csv";
    
    // Uncomment this if you need to generate a fresh dataset
    generate_dataset(dataset_size, 30, dataset_file);

    std::cout << "Loading dataset...\n";
    std::vector<State> dataset = load_dataset(dataset_file);
    std::cout << "Loaded " << dataset.size() << " states.\n";

    std::vector<ExperimentConfig> experiments;
    
    std::array<AgentType, 5> all_agent_types = {
        AgentType::LeafParallel,
        AgentType::RootParallel,
        AgentType::LockFree,
        AgentType::VirtualLoss,
        AgentType::WuUct
    };

    for (int sims_mult: all_sims_mults) {
        experiments.push_back({AgentType::Sequential, sims * sims_mult, 1});
        for (AgentType agent_type: all_agent_types) {
            for (int num_threads: all_num_threads) {
                experiments.push_back({agent_type, sims * sims_mult, num_threads});
            }
        }
    }

    bool first_raw_save = true;
    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < experiments.size(); ++i) {
        auto res = run_experiment(experiments[i], dataset, i + 1, experiments.size());
        save_raw_csv(res, first_raw_save);
        first_raw_save = false;
    }

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    std::cout << "\nTotal time elapsed for all experiments: " 
              << elapsed_seconds.count() << " seconds\n";
    std::cout << "Data saved to results_raw.csv. Ready for Python Oracle Evaluation.\n";

    return 0;
}