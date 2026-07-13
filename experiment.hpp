/*
 * Module: Experiment Definitions
 *
 * Defines the configuration and result structures for the unified 
 * Oracle benchmark suite. Evaluates single-turn execution on a static dataset
 * and computes continuous accuracy natively in C++.
 */

#pragma once

#include "agent.hpp"
#include <string>
#include <vector>
#include <array>

enum class AgentType {
    Sequential, LeafParallel, RootParallel,
    LockFree, VirtualLoss, WuUct
};

struct ExperimentConfig {
    AgentType type;
    int simulations;
    int num_threads;    
};

struct StateEvalResult {
    int state_id;
    int chosen_move_idx;
    double execution_time_ms;
    double oracle_score; // NEW: The continuous metric
};

struct ExperimentResult {
    int config_id;
    int n_size;
    std::string agent_name;
    int simulations;
    int num_threads;
    std::vector<StateEvalResult> evals;
};

ExperimentResult run_experiment(const ExperimentConfig& config, 
                                const std::vector<State>& dataset, 
                                const std::vector<std::array<double, u_SIZE>>& oracle_cache,
                                int config_num, int total_configs);

void save_raw_csv(const ExperimentResult& r, bool is_first);
void save_summary_csv(const std::vector<ExperimentResult>& all_results);