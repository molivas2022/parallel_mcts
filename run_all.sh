#!/bin/bash
# File: run_all.sh

set -e

echo ""
echo "========================================="
echo " STARTING PIPELINE: ORACLE EVAL (N=15)"
echo "========================================="
make clean
make N_SIZE=13 -j4
./main --oracle

echo "========================================="
echo " STARTING PIPELINE: MATCH BENCHMARK (N=11)"
echo "========================================="
make clean
make N_SIZE=11 -j4
./main --match

echo ""
echo "========================================="
echo " ALL EXPERIMENTS COMPLETE."
echo " Run 'python3 plot.py' to visualize."
echo "========================================="