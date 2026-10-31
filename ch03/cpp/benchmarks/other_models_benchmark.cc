// Microbenchmarks for the trinomial model and the binomial vs. lognormal
// comparison (m4f/other_models.h).
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/other_models.h"

namespace m4f::ch03 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

// Reproducible values in [lo, hi).
std::vector<double> Uniform(double lo, double hi, unsigned seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> dist(lo, hi);
  std::vector<double> v(kNumInputs);
  for (double& x : v) x = dist(rng);
  return v;
}

void BM_TrinomialRiskNeutralProbs(benchmark::State& state) {
  const TrinomialModel model{0.1, 0.0, -0.1, 0.02};
  const std::vector<double> p_mid = Uniform(0.0, model.MaxMiddleProb(), 1);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(model.GetRiskNeutralProbs(p_mid[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_TrinomialRiskNeutralProbs);

void BM_NormCdf(benchmark::State& state) {
  const std::vector<double> x = Uniform(-4.0, 4.0, 2);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(NormCdf(x[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_NormCdf);

// Steps in the binomial approximation of a one-year horizon.
void Steps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 1000, 10000}) b->Arg(n);
}

void BM_BinomialExpectedPrice(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const std::vector<double> sigma = Uniform(0.1, 0.4, 3);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        BinomialExpectedPrice(100.0, 0.05, sigma[i], 1.0, n));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_BinomialExpectedPrice)->Apply(Steps);

void BM_LognormalExpectedPrice(benchmark::State& state) {
  const std::vector<double> sigma = Uniform(0.1, 0.4, 3);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        LognormalExpectedPrice(100.0, 0.05, sigma[i], 1.0));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_LognormalExpectedPrice);

void BM_BinomialProbAbove(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const std::vector<double> sigma = Uniform(0.1, 0.4, 3);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        BinomialProbAbove(100.0, 0.05, sigma[i], 1.0, n, 110.0));
    i = (i + 1) & (kNumInputs - 1);
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_BinomialProbAbove)->Apply(Steps)->Complexity(benchmark::oN);

void BM_LognormalProbAbove(benchmark::State& state) {
  const std::vector<double> sigma = Uniform(0.1, 0.4, 3);
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        LognormalProbAbove(100.0, 0.05, sigma[i], 1.0, 110.0));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_LognormalProbAbove);

}  // namespace
}  // namespace m4f::ch03
