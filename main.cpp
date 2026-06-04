#include "experiment.hpp"
#include "agent.hpp"

#include <vector>

int main() {
    int matches = 1;
    SequentialAgent baseline_agent(10); 

    std::vector<ExperimentConfig> experiments = {
        {AgentType::Sequential, 10000, 1},
        {AgentType::LeafParallel, 10000, 4},
        {AgentType::RootParallel, 10000, 4}
    };

    std::vector<ExperimentResult> all_results;
    bool first_save = true;

    for (size_t i = 0; i < experiments.size(); ++i) {
        auto res = run_experiment(experiments[i], matches, baseline_agent, all_results, i + 1, experiments.size());
        all_results.push_back(res);
        
        save_to_csv(res, first_save);
        first_save = false;
    }

    print_final_summary(all_results);
    return 0;
}