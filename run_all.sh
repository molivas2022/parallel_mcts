#!/bin/bash
# File: run_all.sh

set -e

echo ""
echo " STARTING ORACLE EVAL (N=13)"
make clean
make N_SIZE=13 -j4
./main --oracle

echo ""
echo " STARTING MATCH EVAL (N=11)"
make clean
make N_SIZE=11 -j4
./main --match

echo ""
echo " ALL EXPERIMENTS COMPLETE"