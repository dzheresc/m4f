// Microbenchmarks for binomial option pricing (m4f/binomial_pricing.h): the
// full option tree, the closed-form sums, the CRR call formula and the early
// exercise region. Parameters follow a one-year CRR tree with r = 5% and
// sigma = 20%, at the money.
//
// BinomialSum, EuropeanPriceSum and CrrCall stop at N = 1000: past about
// N = 1030 their binomial coefficient overflows (see RESULTS.md).
#include <algorithm>
#include <cmath>

#include "benchmark/benchmark.h"
#include "m4f/binomial_pricing.h"

namespace m4f::ch08 {
namespace {

struct CrrParams {
  double u;
  double d;
  double r;
};

CrrParams Crr(int num_steps) {
  const double tau = 1.0 / num_steps;
  return {std::exp(0.2 * std::sqrt(tau)) - 1,
          std::exp(-0.2 * std::sqrt(tau)) - 1, std::exp(0.05 * tau) - 1};
}

double Call(double s) { return std::max(s - 100.0, 0.0); }
double Put(double s) { return std::max(100.0 - s, 0.0); }

void TreeSteps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 500, 2000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared)->Unit(benchmark::kMicrosecond);
}

void SumSteps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 1000}) b->Arg(n);
  b->Complexity(benchmark::oN);
}

void BM_OptionTreeEuropean(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const CrrParams p = Crr(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(OptionTree(100.0, p.u, p.d, p.r, n, Put));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_OptionTreeEuropean)->Apply(TreeSteps);

void BM_OptionTreeAmerican(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const CrrParams p = Crr(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(OptionTree(100.0, p.u, p.d, p.r, n, Put, true));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_OptionTreeAmerican)->Apply(TreeSteps);

void BM_EarlyExerciseNodes(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const CrrParams p = Crr(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(EarlyExerciseNodes(100.0, p.u, p.d, p.r, n, Put));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_EarlyExerciseNodes)->Apply(TreeSteps);

void BM_ReplicatingPosition(benchmark::State& state) {
  const CrrParams p = Crr(100);
  const Tree tree = OptionTree(100.0, p.u, p.d, p.r, 100, Call);
  int k = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ReplicatingPosition(100.0, p.u, p.d, p.r, tree, 50, k));
    k = (k + 1) % 51;
  }
}
BENCHMARK(BM_ReplicatingPosition);

void BM_BinomialSum(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) benchmark::DoNotOptimize(BinomialSum(0.5, n, n / 2));
  state.SetComplexityN(n);
}
BENCHMARK(BM_BinomialSum)->Apply(SumSteps);

void BM_EuropeanPriceSum(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const CrrParams p = Crr(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(EuropeanPriceSum(100.0, p.u, p.d, p.r, n, Call));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_EuropeanPriceSum)->Apply(SumSteps);

void BM_CrrCall(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const CrrParams p = Crr(n);
  for (auto _ : state) {
    benchmark::DoNotOptimize(CrrCall(100.0, 100.0, p.u, p.d, p.r, n));
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_CrrCall)->Apply(SumSteps);

}  // namespace
}  // namespace m4f::ch08
