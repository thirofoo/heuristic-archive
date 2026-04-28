#!/bin/bash
set -e

SEED=$(printf "%04d" "$1")

g++ -O3 -std=c++17 ./src/solve_a.cpp -o ./solve_a

mkdir -p ../tools/outA ../tools/errA

./solve_a < "../tools/inA/${SEED}.txt" > "../tools/outA/${SEED}.txt" 2> "../tools/errA/${SEED}.txt"

../tools/target/release/score "../tools/inA/${SEED}.txt" "../tools/outA/${SEED}.txt"
