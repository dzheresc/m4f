// Microbenchmarks for interest rate derivatives on the short-rate lattice
// (m4f/derivatives.h): bond options, swaps, caps, floors and caplets.
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/derivatives.h"

namespace m4f::ch11 {
namespace {

constexpr double kTau = 1.0 / 12;

struct Model {
  Lattice rates;
  Lattice probs;
};

Model MakeModel(int n) {
  return {RateLattice(0.05, 0.002, -0.002, n), ConstantProbs(0.5, n)};
}

void Steps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 1000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared);
}

// An option expiring halfway through the life of the bond.
void RunBondOption(benchmark::State& state, bool american) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        BondOption(m.rates, m.probs, kTau, n, n / 2, 0.97, false, american));
  }
  state.SetComplexityN(n);
}

void BM_BondOptionEuropean(benchmark::State& state) {
  RunBondOption(state, false);
}
BENCHMARK(BM_BondOptionEuropean)->Apply(Steps);

void BM_BondOptionAmerican(benchmark::State& state) {
  RunBondOption(state, true);
}
BENCHMARK(BM_BondOptionAmerican)->Apply(Steps);

void BM_PayerSwap(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(PayerSwap(m.rates, m.probs, kTau, n, 0.004));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_PayerSwap)->Apply(Steps);

void BM_Cap(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Cap(m.rates, m.probs, kTau, n, 0.004));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_Cap)->Apply(Steps);

void BM_Floor(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Floor(m.rates, m.probs, kTau, n, 0.004));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_Floor)->Apply(Steps);

// One caplet fixing at the last step: a full lattice for one number.
void BM_Caplet(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(Caplet(m.rates, m.probs, kTau, n - 1, 0.004));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_Caplet)->Apply(Steps);

void BM_SwapRate(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const Model m = MakeModel(n);
  const std::vector<double> prices = BondPrices(m.rates, m.probs, kTau, n);
  for (auto _ : state) benchmark::DoNotOptimize(SwapRate(prices));
}
BENCHMARK(BM_SwapRate)->Arg(10)->Arg(100);

}  // namespace
}  // namespace m4f::ch11
