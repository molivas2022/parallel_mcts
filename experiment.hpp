#pragma once

#include "agent.hpp"

#include <string>
#include <vector>

enum class AgentType {
    Sequential,
    LeafParallel
};

struct ExperimentConfig {
    AgentType type;
    int simulations;
    int num_threads;    // = 1 for Sequential
};

struct ExperimentResult {
    int n_size;
    int matches;
    std::string agent_name;
    int simulations;
    int num_threads;
    double winrate;
    double avg_turns;
    double time_seconds;
};

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                int matches, 
                                Agent& baseline_agent, 
                                const std::vector<ExperimentResult>& past_results, 
                                int config_num, int total_configs);

void save_to_csv(const ExperimentResult& r, bool is_first);
void print_final_summary(const std::vector<ExperimentResult>& all_results);