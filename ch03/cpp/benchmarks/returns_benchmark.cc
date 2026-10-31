// Microbenchmarks for returns and moments (m4f/returns.h) on daily price
// paths of one year (252 days) and ten years (2520 days).
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/returns.h"

namespace m4f::ch03 {
namespace {

// A reproducible lognormal daily path starting at 100.
std::vector<double> Path(std::size_t days) {
  std::mt19937_64 rng(days);
  std::normal_distribution<double> shock(0.0003, 0.01);
  std::vector<double> path{100.0};
  for (std::size_t i = 0; i < days; ++i) {
    path.push_back(path.back() * std::exp(shock(rng)));
  }
  return path;
}

void Days(benchmark::internal::Benchmark* b) {
  for (int n : {252, 2520}) b->Arg(n);
}

void BM_Returns(benchmark::State& state) {
  const std::vector<double> path =
      Path(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(Returns(path));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Returns)->Apply(Days);

void BM_LogReturns(benchmark::State& state) {
  const std::vector<double> path =
      Path(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(LogReturns(path));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_LogReturns)->Apply(Days);

void BM_CompoundReturn(benchmark::State& state) {
  const std::vector<double> returns =
      Returns(Path(static_cast<std::size_t>(state.range(0))));
  for (auto _ : state) benchmark::DoNotOptimize(CompoundReturn(returns));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_CompoundReturn)->Apply(Days);

// Mean and variance of the returns with equal weights.
void BM_ExpectationVariance(benchmark::State& state) {
  const std::vector<double> returns =
      Returns(Path(static_cast<std::size_t>(state.range(0))));
  const std::vector<double> probs(returns.size(), 1.0 / returns.size());
  for (auto _ : state) {
    benchmark::DoNotOptimize(Expectation(returns, probs));
    benchmark::DoNotOptimize(Variance(returns, probs));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ExpectationVariance)->Apply(Days);

}  // namespace
}  // namespace m4f::ch03
