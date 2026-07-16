/*
 * Defines the configuration and result structures for both the match benchmark and the oracle benchmark
 */

#pragma once

#include "agent.hpp"
#include "env.hpp"

#include <string>
#include <vector>
#include <array>
#include <memory>

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

// Match pipeline structures

struct MatchResult {
    int match_num;
    bool test_is_p1;
    bool test_won;
    int game_turns;
    int test_agent_turns;
    double test_mcts_time;
    double baseline_mcts_time;
};

struct MatchExperimentResult {
    int config_id;
    int n_size;
    std::string agent_name;
    int simulations;
    int num_threads;
    std::vector<MatchResult> matches;
};

// Oracle pipeline structures

struct StateEvalResult {
    int state_id;
    int chosen_move_idx;
    double execution_time_ms;
    double oracle_score; 
};

struct OracleExperimentResult {
    int config_id;
    int n_size;
    std::string agent_name;
    int simulations;
    int num_threads;
    std::vector<StateEvalResult> evals;
};

// Runners

MatchExperimentResult run_match_experiment(const ExperimentConfig& config, 
                                           int matches, 
                                           Agent& baseline_agent, 
                                           const std::vector<MatchExperimentResult>& past_results, 
                                           int config_num, int total_configs);

OracleExperimentResult run_oracle_experiment(const ExperimentConfig& config, 
                                             const std::vector<State>& dataset, 
                                             const std::vector<std::array<double, u_SIZE>>& oracle_cache,
                                             int repetitions,
                                             int config_num, int total_configs);

// match csvs

void save_match_raw_csv(const MatchExperimentResult& r, bool is_first);
// void save_match_summary_csv(const std::vector<MatchExperimentResult>& all_results);
void print_match_final_summary(const std::vector<MatchExperimentResult>& all_results);

// oracle csvs

void save_oracle_raw_csv(const OracleExperimentResult& r, bool is_first);
// void save_oracle_summary_csv(const std::vector<OracleExperimentResult>& all_results);