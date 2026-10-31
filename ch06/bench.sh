#!/usr/bin/env bash
# Runs the microbenchmarks; the C++ ones are built in Release mode first.
#   ./bench.sh                 wall-clock timings (Google Benchmark)
#   ./bench.sh --callgrind     also instruction counts per iteration (valgrind)
# Extra arguments go to the benchmark binary, e.g. --benchmark_filter=BestCycle.
set -euo pipefail
cd "$(dirname "$0")"

callgrind=0
if [[ "${1:-}" == "--callgrind" ]]; then
  callgrind=1
  shift
fi

cmake -S cpp -B cpp/build-bench -DCMAKE_BUILD_TYPE=Release \
  -DM4F_BUILD_BENCHMARKS=ON >/dev/null
cmake --build cpp/build-bench -j --target ch06_benchmark >/dev/null

# Five repetitions; report mean, median, stddev and coefficient of variation.
cpp/build-bench/ch06_benchmark --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true "$@"

if ((callgrind)); then
  echo
  python3 cpp/benchmarks/callgrind_ir.py cpp/build-bench/ch06_benchmark
fi
