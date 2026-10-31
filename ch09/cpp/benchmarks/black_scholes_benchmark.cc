// Microbenchmarks for the normal distribution helpers and Black-Scholes
// prices and Greeks (m4f/black_scholes.h).
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/black_scholes.h"

namespace m4f::ch09 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

struct Inputs {
  double s;
  double sigma;
  double maturity;
  double prob;
};

const std::vector<Inputs>& Table() {
  static const std::vector<Inputs> table = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> s(60.0, 140.0);
    std::uniform_real_distribution<double> sigma(0.1, 0.5);
    std::uniform_real_distribution<double> maturity(0.05, 3.0);
    std::uniform_real_distribution<double> prob(0.001, 0.999);
    std::vector<Inputs> v;
    for (std::size_t i = 0; i < kNumInputs; ++i) {
      v.push_back({s(rng), sigma(rng), maturity(rng), prob(rng)});
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

void BM_NormPdf(benchmark::State& state) {
  RunOverInputs(state,
                [](const Inputs& in) { return NormPdf(in.s / 20.0 - 5.0); });
}
BENCHMARK(BM_NormPdf);

void BM_NormInv(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) { return NormInv(in.prob); });
}
BENCHMARK(BM_NormInv);

void BM_BsPriceCall(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsPrice(in.s, 100.0, 0.05, in.sigma, in.maturity, true);
  });
}
BENCHMARK(BM_BsPriceCall);

void BM_BsPricePut(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsPrice(in.s, 100.0, 0.05, in.sigma, in.maturity, false);
  });
}
BENCHMARK(BM_BsPricePut);

void BM_BsGreeksCall(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsGreeks(in.s, 100.0, 0.05, in.sigma, in.maturity, true);
  });
}
BENCHMARK(BM_BsGreeksCall);

void BM_BsGreeksPut(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return BsGreeks(in.s, 100.0, 0.05, in.sigma, in.maturity, false);
  });
}
BENCHMARK(BM_BsGreeksPut);

}  // namespace
}  // namespace m4f::ch09
