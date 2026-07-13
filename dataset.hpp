/*
 * Module: Dataset Generator
 *
 * Utility to generate static, mid-game Hex board states for offline 
 * Oracle evaluation. It plays a fixed number of random moves to ensure 
 * the tree is adequately complex, discarding any games that accidentally 
 * finish too early.
 */

#pragma once

#include "env.hpp"
#include <string>
#include <vector>

void generate_dataset(int num_states, int moves_per_game, const std::string& filename);
std::vector<State> load_dataset(const std::string& filename);