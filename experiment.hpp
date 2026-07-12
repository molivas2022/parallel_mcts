/*
* Module: Experiment Definitions
*
* Defines the configuration and result structures for the benchmark suite.
* The results are split into two tiers: MatchResult (raw data per game) 
* and ExperimentResult (aggregated data per agent configuration).
*/

#pragma once

#include "agent.hpp"

#include <string>
#include <vector>

enum class AgentType {
    Sequential,
    LeafParallel,
    RootParallel,
    LockFree,
    VirtualLoss,
    WuUct
};

struct ExperimentConfig {
    AgentType type;
    int simulations;
    int num_threads;    
};

struct MatchResult {
    int match_num;
    bool test_is_p1;
    bool test_won;
    int game_turns;
    int test_agent_turns;
    double test_mcts_time;
    double baseline_mcts_time;
};

struct ExperimentResult {
    int config_id;
    int n_size;
    std::string agent_name;
    int simulations;
    int num_threads;
    std::vector<MatchResult> matches;
};

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                int matches, 
                                Agent& baseline_agent, 
                                const std::vector<ExperimentResult>& past_results, 
                                int config_num, int total_configs);

void save_raw_csv(const ExperimentResult& r, bool is_first);
void save_summary_csv(const std::vector<ExperimentResult>& all_results);
void print_final_summary(const std::vector<ExperimentResult>& all_results);