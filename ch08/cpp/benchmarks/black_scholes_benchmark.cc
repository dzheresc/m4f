// Microbenchmarks for the Black-Scholes formulas and the CRR tree price that
// converges to them (m4f/black_scholes.h).
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/black_scholes.h"

namespace m4f::ch08 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

struct Inputs {
  double s;
  double sigma;
  double maturity;
};

const std::vector<Inputs>& Table() {
  static const std::vector<Inputs> table = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> s(60.0, 140.0);
    std::uniform_real_distribution<double> sigma(0.1, 0.5);
    std::uniform_real_distribution<double> maturity(0.05, 3.0);
    std::vector<Inputs> v;
    for (std::size_t i = 0; i < kNumInputs; ++i) {
      v.push_back({s(rng), sigma(rng), maturity(rng)});
    }
    return v;
  }();
  return table;
}

template <typename Op>
void RunOverInputs(benchmark::State& state, Op op) {
  const std::vector<Inputs>& table = Table();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(table[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}

void BM_NormCdf(benchmark::State& state) {
  RunOverInputs(state,
                [](const Inputs& in) { return NormCdf(in.s / 20.0 - 5.0); });
}
BENCHMARK(BM_NormCdf);

void BM_BsCall(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsCall(in.s, 100.0, 0.05, in.sigma, in.maturity);
  });
}
BENCHMARK(BM_BsCall);

void BM_BsPut(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsPut(in.s, 100.0, 0.05, in.sigma, in.maturity);
  });
}
BENCHMARK(BM_BsPut);

void BM_BsCallDelta(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsCallDelta(in.s, 100.0, 0.05, in.sigma, in.maturity);
  });
}
BENCHMARK(BM_BsCallDelta);

void TreeSteps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 500, 2000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared)->Unit(benchmark::kMicrosecond);
}

void BM_CrrTreePriceEuropean(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        CrrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, n, true));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_CrrTreePriceEuropean)->Apply(TreeSteps);

void BM_CrrTreePriceAmerican(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        CrrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, n, false, true));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_CrrTreePriceAmerican)->Apply(TreeSteps);

}  // namespace
}  // namespace m4f::ch08
