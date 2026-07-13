#!/bin/bash

# Exit immediately if any command fails
set -e

echo "========================================================"
echo "      MCTS Hex Benchmark: MoHex Oracle Pipeline         "
echo "========================================================"

# 1. Compilation
echo -e "\n>>> [1/4] Compiling C++ project..."
make clean
# Use all available cores for compilation
if command -v nproc > /dev/null; then
    make -j$(nproc)
elif command -v sysctl > /dev/null; then
    make -j$(sysctl -n hw.ncpu)
else
    make
fi

# 2. Dataset Check
echo -e "\n>>> [2/4] Verifying Dataset..."
if [ ! -f "dataset.csv" ]; then
    echo "CRITICAL ERROR: 'dataset.csv' not found."
    echo "Please uncomment 'generate_dataset(100, 30, dataset_file);' in main.cpp"
    echo "compile, run it once to generate the data, then comment it out again."
    exit 1
else
    echo "'dataset.csv' found. Proceeding to benchmark."
fi

# 3. C++ Benchmark
echo -e "\n>>> [3/4] Running C++ Parallel Benchmark..."
./main

# 4. Python Oracle Evaluation
echo -e "\n>>> [4/4] Running MoHex Oracle Evaluation..."
uv run evaluate_oracle.py

# 5. Visualization
echo -e "\n>>> [5/5] Launching Visualization Dashboard..."
uv run plot.py

echo -e "\n========================================================"
echo " Pipeline Complete!"
echo "========================================================"