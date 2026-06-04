#pragma once

#include <string>

enum class AgentType {
    Sequential,
    LeafParallel
};

struct ExperimentConfig {
    AgentType type;
    int iters;
    int num_threads; // 1 for sequential
};

struct ExperimentResult {
    int n_size;
    int matches;
    std::string agent_name;
    int iters;
    int num_threads;
    double winrate;
    double avg_turns;
    double time_seconds;
};