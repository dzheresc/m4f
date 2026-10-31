#!/usr/bin/env bash
# Runs the microbenchmarks; the C++ ones are built in Release mode first.
#   ./bench.sh                 wall-clock timings (Google Benchmark)
#   ./bench.sh --callgrind     also instruction counts per iteration (valgrind)
#   ./bench.sh --kotlin [re]   Kotlin microbenchmarks (JMH), names matching re
#   ./bench.sh --python [s]    Python microbenchmarks (timeit), names containing s
#   ./bench.sh --rust [s]      Rust microbenchmarks (Criterion), names containing s
#   ./bench.sh --rust-callgrind [re]  Rust instruction counts per iteration
#   ./bench.sh --compare [re]  Rust vs C++ table (time and instructions)
# Extra arguments go to the benchmark binary, e.g. --benchmark_filter=BestCycle.
set -euo pipefail
cd "$(dirname "$0")"

case "${1:-}" in
  --kotlin)
    cd kotlin
    exec ./gradlew -q --console=plain jmh ${2:+-Pbench="$2"}
    ;;
  --python)
    exec python3 python/bench_ch02.py "${@:2}"
    ;;
  --rust)
    cd rust
    exec cargo bench -q --bench ch02_benchmark -- ${2:+"$2"}
    ;;
  --rust-callgrind)
    (cd rust && cargo build -q --profile bench --example bench_ir)
    exec python3 rust/benches/callgrind_ir.py \
      rust/target/release/examples/bench_ir ${2:+"$2"}
    ;;
  --compare)
    cmake -S cpp -B cpp/build-bench -DCMAKE_BUILD_TYPE=Release \
      -DM4F_BUILD_BENCHMARKS=ON >/dev/null
    cmake --build cpp/build-bench -j --target ch02_benchmark >/dev/null
    (cd rust && cargo bench -q --bench ch02_benchmark -- ${2:+"$2"} >/dev/null 2>&1 &&
      cargo build -q --profile bench --example bench_ir)
    exec python3 rust/benches/compare_cpp.py cpp/build-bench/ch02_benchmark \
      rust/target/release/examples/bench_ir rust/target/criterion ${2:+"$2"}
    ;;
esac

callgrind=0
if [[ "${1:-}" == "--callgrind" ]]; then
  callgrind=1
  shift
fi

cmake -S cpp -B cpp/build-bench -DCMAKE_BUILD_TYPE=Release \
  -DM4F_BUILD_BENCHMARKS=ON >/dev/null
cmake --build cpp/build-bench -j --target ch02_benchmark >/dev/null

# Five repetitions; report mean, median, stddev and coefficient of variation.
cpp/build-bench/ch02_benchmark --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true "$@"

if ((callgrind)); then
  echo
  python3 cpp/benchmarks/callgrind_ir.py cpp/build-bench/ch02_benchmark
fi
