/*
 * Utility to generate static, mid-game Hex board states for offline Oracle evaluation.
 */

#pragma once

#include "env.hpp"
#include <string>
#include <vector>

void generate_dataset(int num_states, int moves_per_game, const std::string& filename);
std::vector<State> load_dataset(const std::string& filename);