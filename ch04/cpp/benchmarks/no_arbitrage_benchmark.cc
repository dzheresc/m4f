// Microbenchmarks for no-arbitrage checks, risk-neutral probabilities and the
// expected discounted value of a strategy over a binomial tree
// (m4f/no_arbitrage.h).
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/no_arbitrage.h"

namespace m4f::ch04 {
namespace {

// Scenario returns: 2 (binomial), 3 (Section 4.1.4) and 100 scenarios.
Vec Scenarios(std::size_t n) {
  std::mt19937_64 rng(n);
  std::uniform_real_distribution<double> k(-0.2, 0.25);
  Vec returns(n);
  for (double& x : returns) x = k(rng);
  return returns;
}

void ScenarioCounts(benchmark::internal::Benchmark* b) {
  for (int n : {2, 3, 100}) b->Arg(n);
}

void BM_IsArbitrageFree(benchmark::State& state) {
  const Vec returns = Scenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(IsArbitrageFree(returns, 0.02));
  }
}
BENCHMARK(BM_IsArbitrageFree)->Apply(ScenarioCounts);

void BM_ArbitrageStrategy(benchmark::State& state) {
  const Vec returns = Scenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    // r above every return: the short-stock strategy.
    benchmark::DoNotOptimize(ArbitrageStrategy(returns, 0.3, 100.0, 100.0));
  }
}
BENCHMARK(BM_ArbitrageStrategy)->Apply(ScenarioCounts);

void BM_RiskNeutralProbs(benchmark::State& state) {
  const Vec returns = Scenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(RiskNeutralProbs(returns, 0.02));
  }
}
BENCHMARK(BM_RiskNeutralProbs)->Apply(ScenarioCounts);

void BM_OneStepValues(benchmark::State& state) {
  const Vec returns = Scenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        OneStepValues(-1.0, 1.0, returns, 0.02, 100.0, 100.0));
  }
}
BENCHMARK(BM_OneStepValues)->Apply(ScenarioCounts);

// The report's contrarian rule: x(n) = 1 + number of down moves so far.
double Contrarian(int /*n*/, const std::vector<int>& moves) {
  double downs = 0.0;
  for (int move : moves) downs += move == 0 ? 1.0 : 0.0;
  return 1.0 + downs;
}

void TreeDepths(benchmark::internal::Benchmark* b) {
  b->DenseRange(4, 16, 4)->Unit(benchmark::kMicrosecond);
}

// Enumerates all 2^N paths, so the cost doubles with each step. N = 4 is the
// report's example.
void BM_ExpectedDiscountedValue(benchmark::State& state) {
  const int num_steps = static_cast<int>(state.range(0));
  const double q = (0.03 - -0.05) / (0.10 - -0.05);
  for (auto _ : state) {
    benchmark::DoNotOptimize(ExpectedDiscountedValue(
        100.0, 0.10, -0.05, 0.03, num_steps, 100.0, Contrarian, q));
  }
  state.SetItemsProcessed(state.iterations() * (int64_t{1} << num_steps));
}
BENCHMARK(BM_ExpectedDiscountedValue)->Apply(TreeDepths);

}  // namespace
}  // namespace m4f::ch04
