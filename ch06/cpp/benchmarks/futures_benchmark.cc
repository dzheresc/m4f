// Microbenchmarks for futures prices on a binomial tree, the margin account
// and minimum-variance hedging (m4f/futures.h).
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/futures.h"

namespace m4f::ch06 {
namespace {

void TreeDepths(benchmark::internal::Benchmark* b) {
  for (int n : {3, 10, 100, 1000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared);
}

// Builds and fills the whole (N+1)(N+2)/2-node tree: O(N^2).
void BM_FuturesTree(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(FuturesTree(100.0, 0.02, -0.01, 0.005, n));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_FuturesTree)->Apply(TreeDepths);

// A reproducible daily futures price path starting at 100.
Vec Path(std::size_t days) {
  std::mt19937_64 rng(days);
  std::normal_distribution<double> shock(0.0, 0.015);
  Vec path{100.0};
  for (std::size_t i = 0; i < days; ++i) {
    path.push_back(path.back() * std::exp(shock(rng)));
  }
  return path;
}

void BM_MarginAccount(benchmark::State& state) {
  const Vec prices = Path(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(MarginAccount(prices, 100.0, 1000.0, 700.0));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MarginAccount)->Arg(252)->Arg(2520);

// Spot and futures price changes over n equally likely scenarios.
struct Scenarios {
  Vec ds;
  Vec df;
  Vec p;
};

Scenarios MakeScenarios(std::size_t n) {
  std::mt19937_64 rng(n);
  std::normal_distribution<double> change(0.0, 2.0);
  Scenarios s;
  for (std::size_t i = 0; i < n; ++i) {
    s.df.push_back(change(rng));
    s.ds.push_back(0.9 * s.df.back() + 0.3 * change(rng));
  }
  s.p.assign(n, 1.0 / static_cast<double>(n));
  return s;
}

void BM_MinVarianceHedgeRatio(benchmark::State& state) {
  const Scenarios s = MakeScenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(MinVarianceHedgeRatio(s.ds, s.df, s.p));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MinVarianceHedgeRatio)->Arg(3)->Arg(100)->Arg(10000);

void BM_HedgedStd(benchmark::State& state) {
  const Scenarios s = MakeScenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(HedgedStd(0.9, s.ds, s.df, s.p));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HedgedStd)->Arg(3)->Arg(100)->Arg(10000);

}  // namespace
}  // namespace m4f::ch06
