// Microbenchmarks for forward prices and values (m4f/forwards.h).
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/forwards.h"

namespace m4f::ch06 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

// Reproducible rates in [0, 10%).
const std::vector<double>& Rates() {
  static const std::vector<double> rates = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> rate(0.0, 0.10);
    std::vector<double> v(kNumInputs);
    for (double& r : v) r = rate(rng);
    return v;
  }();
  return rates;
}

template <typename Op>
void RunOverRates(benchmark::State& state, Op op) {
  const std::vector<double>& rates = Rates();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(rates[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
}

void BM_ForwardPrice(benchmark::State& state) {
  RunOverRates(state, [](double r) { return ForwardPrice(100.0, r, 0.75); });
}
BENCHMARK(BM_ForwardPrice);

void BM_ForwardPriceYield(benchmark::State& state) {
  RunOverRates(
      state, [](double r) { return ForwardPriceYield(100.0, r, 0.02, 0.75); });
}
BENCHMARK(BM_ForwardPriceYield);

void BM_ForwardArbitrageProfit(benchmark::State& state) {
  RunOverRates(state, [](double r) {
    return ForwardArbitrageProfit(105.0, 100.0, r, 0.75);
  });
}
BENCHMARK(BM_ForwardArbitrageProfit);

void BM_LongForwardValue(benchmark::State& state) {
  RunOverRates(state,
               [](double r) { return LongForwardValue(100.0, 104.0, r, 0.5); });
}
BENCHMARK(BM_LongForwardValue);

// Quarterly dividends over 1 year (4) and 25 years (100).
void BM_ForwardPriceDividends(benchmark::State& state) {
  std::vector<std::pair<double, double>> dividends;
  for (int i = 1; i <= state.range(0); ++i)
    dividends.emplace_back(0.5, i / 4.0);
  const double maturity = static_cast<double>(state.range(0)) / 4.0;
  RunOverRates(state, [&](double r) {
    return ForwardPriceDividends(100.0, r, maturity, dividends);
  });
}
BENCHMARK(BM_ForwardPriceDividends)->Arg(4)->Arg(100);

}  // namespace
}  // namespace m4f::ch06
