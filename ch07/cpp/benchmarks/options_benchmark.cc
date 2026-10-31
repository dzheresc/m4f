// Microbenchmarks for payoffs, put-call parity and price bounds
// (m4f/options.h). Each is a few operations and at most one exp, so the
// inputs cycle through a table of strikes.
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/options.h"

namespace m4f::ch07 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

const std::vector<double>& Strikes() {
  static const std::vector<double> strikes = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> strike(70.0, 130.0);
    std::vector<double> v(kNumInputs);
    for (double& x : v) x = strike(rng);
    return v;
  }();
  return strikes;
}

template <typename Op>
void RunOverStrikes(benchmark::State& state, Op op) {
  const std::vector<double>& strikes = Strikes();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(strikes[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}

void BM_CallPayoff(benchmark::State& state) {
  RunOverStrikes(state, [](double x) { return CallPayoff(100.0, x); });
}
BENCHMARK(BM_CallPayoff);

void BM_Profit(benchmark::State& state) {
  RunOverStrikes(state, [](double x) {
    return Profit(PutPayoff(100.0, x), 5.0, 0.05, 0.5);
  });
}
BENCHMARK(BM_Profit);

void BM_ParityGap(benchmark::State& state) {
  RunOverStrikes(state, [](double x) {
    return ParityGap(10.0, 5.0, 100.0, x, 0.05, 0.5, 1.0);
  });
}
BENCHMARK(BM_ParityGap);

void BM_EuropeanBounds(benchmark::State& state) {
  RunOverStrikes(state, [](double x) {
    return BoundViolation(8.0, EuropeanCallBounds(100.0, x, 0.05, 0.5)) +
           BoundViolation(4.0, EuropeanPutBounds(100.0, x, 0.05, 0.5));
  });
}
BENCHMARK(BM_EuropeanBounds);

void BM_AmericanParityBounds(benchmark::State& state) {
  RunOverStrikes(state, [](double x) {
    const Bounds b = AmericanParityBounds(100.0, x, 0.05, 0.5);
    return b.lo + b.hi;
  });
}
BENCHMARK(BM_AmericanParityBounds);

void BM_ButterflyGap(benchmark::State& state) {
  RunOverStrikes(state, [](double x) {
    return ButterflyGap(12.0, 7.0, 3.0, x - 10.0, x, x + 10.0);
  });
}
BENCHMARK(BM_ButterflyGap);

}  // namespace
}  // namespace m4f::ch07
