#include "experiment.hpp"
#include "agent.hpp"

#include <vector>

int main() {
    int matches = 10; // Matches per experiment
    int sims = 10000;
    SequentialAgent baseline_agent(sims); 

    std::vector<ExperimentConfig> experiments = {
        {AgentType::Sequential, sims, 1},
        // {AgentType::LeafParallel, sims, 4},
        // {AgentType::RootParallel, sims, 4},
        // {AgentType::LockFree, sims, 4},
        // {AgentType::VirtualLoss, sims, 4}
    };

    std::vector<ExperimentResult> all_results;
    bool first_raw_save = true;

    for (size_t i = 0; i < experiments.size(); ++i) {
        auto res = run_experiment(experiments[i], matches, baseline_agent, all_results, i + 1, experiments.size());
        all_results.push_back(res);
        
        // Append raw match data as soon as the experiment finishes
        save_raw_csv(res, first_raw_save);
        first_raw_save = false;
    }

    // Generate the calculated summary table
    save_summary_csv(all_results);
    
    print_final_summary(all_results);
    return 0;
}