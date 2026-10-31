// Microbenchmarks for scenario statistics and the two-asset formulas
// (m4f/risk.h).
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/risk.h"

namespace m4f::ch05 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

// Two return series over n equally likely scenarios.
struct Scenarios {
  Vec k1;
  Vec k2;
  Vec p;
};

Scenarios MakeScenarios(std::size_t n) {
  std::mt19937_64 rng(n);
  std::normal_distribution<double> k(0.08, 0.2);
  Scenarios s;
  for (std::size_t i = 0; i < n; ++i) {
    s.k1.push_back(k(rng));
    s.k2.push_back(0.5 * s.k1.back() + k(rng));
  }
  s.p.assign(n, 1.0 / static_cast<double>(n));
  return s;
}

void ScenarioCounts(benchmark::internal::Benchmark* b) {
  for (int n : {3, 100, 10000}) b->Arg(n);
}

void BM_Covariance(benchmark::State& state) {
  const Scenarios s = MakeScenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(Covariance(s.k1, s.k2, s.p));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Covariance)->Apply(ScenarioCounts);

void BM_Correlation(benchmark::State& state) {
  const Scenarios s = MakeScenarios(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(Correlation(s.k1, s.k2, s.p));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Correlation)->Apply(ScenarioCounts);

// Reproducible two-asset inputs.
struct TwoAssets {
  double w1, s1, s2, rho;
};

const std::vector<TwoAssets>& Pairs() {
  static const std::vector<TwoAssets> pairs = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> w(-0.5, 1.5);
    std::uniform_real_distribution<double> s(0.1, 0.4);
    std::uniform_real_distribution<double> rho(-0.9, 0.9);
    std::vector<TwoAssets> v;
    for (std::size_t i = 0; i < kNumInputs; ++i) {
      v.push_back({w(rng), s(rng), s(rng), rho(rng)});
    }
    return v;
  }();
  return pairs;
}

template <typename Op>
void RunOverPairs(benchmark::State& state, Op op) {
  const std::vector<TwoAssets>& pairs = Pairs();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(pairs[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}

void BM_TwoAssetStd(benchmark::State& state) {
  RunOverPairs(state, [](const TwoAssets& a) {
    return TwoAssetStd(a.w1, a.s1, a.s2, a.rho);
  });
}
BENCHMARK(BM_TwoAssetStd);

void BM_MinVarianceWeight(benchmark::State& state) {
  RunOverPairs(state, [](const TwoAssets& a) {
    return MinVarianceWeight(a.s1, a.s2, a.rho);
  });
}
BENCHMARK(BM_MinVarianceWeight);

}  // namespace
}  // namespace m4f::ch05
