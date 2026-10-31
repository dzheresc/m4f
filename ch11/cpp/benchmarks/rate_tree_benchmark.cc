// Microbenchmarks for the short-rate lattice (m4f/rate_tree.h): building the
// lattice, backward induction, bond prices, implied probabilities and the
// money market account. A lattice with N steps has (N+1)(N+2)/2 nodes.
#include <string>

#include "benchmark/benchmark.h"
#include "m4f/rate_tree.h"

namespace m4f::ch11 {
namespace {

// Monthly steps, rates from 5% moving by +/- 0.2% a step.
constexpr double kTau = 1.0 / 12;

Lattice Rates(int n) { return RateLattice(0.05, 0.002, -0.002, n); }

void Steps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 1000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared);
}

void BM_RateLattice(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) benchmark::DoNotOptimize(Rates(n));
  state.SetComplexityN(n);
}
BENCHMARK(BM_RateLattice)->Apply(Steps);

void BM_BondLattice(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Lattice rates = Rates(n);
  const Lattice probs = ConstantProbs(0.5, n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(BondLattice(rates, probs, kTau, n));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_BondLattice)->Apply(Steps);

void BM_ImpliedProbs(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Lattice rates = Rates(n);
  const Lattice bond = BondLattice(rates, ConstantProbs(0.5, n), kTau, n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(ImpliedProbs(bond, rates, kTau));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_ImpliedProbs)->Apply(Steps);

void CurveSteps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 50, 200}) b->Arg(n);
  b->Complexity(benchmark::oNCubed)->Unit(benchmark::kMicrosecond);
}

// The whole zero curve B(0, m) for m = 1..N. Each maturity runs its own
// backward induction, so this is O(N^3).
void BM_BondPrices(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Lattice rates = Rates(n);
  const Lattice probs = ConstantProbs(0.5, n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(BondPrices(rates, probs, kTau, n));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_BondPrices)->Apply(CurveSteps);

void BM_MoneyMarket(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Lattice rates = Rates(n);
  std::string path;
  for (int i = 0; i < n; ++i) path += i % 3 == 0 ? 'u' : 'd';
  for (auto _ : state) {
    benchmark::DoNotOptimize(MoneyMarket(rates, kTau, path));
  }
}
BENCHMARK(BM_MoneyMarket)->Arg(10)->Arg(100)->Arg(1000);

}  // namespace
}  // namespace m4f::ch11
