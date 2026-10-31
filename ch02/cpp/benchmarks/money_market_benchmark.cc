// Microbenchmarks for Section 2.2, the money market (m4f/money_market.h).
//
// CouponBondPrice and YieldToMaturity loop over the coupons, so they are run
// at several bond lengths: 3 coupons (the book's example), 10 and 30 (annual
// bonds), 60 (30 years semi-annual) and 360 (30 years monthly).
#include <cstddef>
#include <optional>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/money_market.h"

namespace m4f::ch02 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

// Reproducible rates in 1% - 15%.
const std::vector<double>& Rates() {
  static const std::vector<double> rates = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> rate(0.01, 0.15);
    std::vector<double> v(kNumInputs);
    for (double& r : v) r = rate(rng);
    return v;
  }();
  return rates;
}

void BM_ZeroCouponPricePeriodic(benchmark::State& state) {
  const std::vector<double>& rates = Rates();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(ZeroCouponPrice(rates[i], 2.0, 2));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_ZeroCouponPricePeriodic);

void BM_ZeroCouponPriceContinuous(benchmark::State& state) {
  const std::vector<double>& rates = Rates();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(ZeroCouponPrice(rates[i], 2.0));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_ZeroCouponPriceContinuous);

void BM_ImpliedRatePeriodic(benchmark::State& state) {
  const std::vector<double>& rates = Rates();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(ImpliedRate(1 - rates[i], 1.0, 2));
    i = (i + 1) & (kNumInputs - 1);
  }
}
BENCHMARK(BM_ImpliedRatePeriodic);

// Coupon counts for the bond benchmarks, with an O(n) complexity fit.
void BondLengths(benchmark::internal::Benchmark* b) {
  for (int n : {3, 10, 30, 60, 360}) b->Arg(n);
  b->Complexity(benchmark::oN);
}

// Per-period yield `y` for coupon bonds of n periods, face 100, coupon 100 y.
void BM_CouponBondPrice(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const std::vector<double>& rates = Rates();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(CouponBondPrice(100.0, 8.0, n, rates[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_CouponBondPrice)->Apply(BondLengths);

void BM_YieldToMaturity(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  // Prices of the same bonds at the generated yields, so every solve has a
  // different target.
  std::vector<double> prices;
  prices.reserve(kNumInputs);
  for (double y : Rates()) prices.push_back(CouponBondPrice(100.0, 8.0, n, y));
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(YieldToMaturity(prices[i], 100.0, 8.0, n));
    i = (i + 1) & (kNumInputs - 1);
  }
  state.SetComplexityN(n);
}
BENCHMARK(BM_YieldToMaturity)->Apply(BondLengths);

void BM_MoneyMarketRollover(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  std::vector<double> bond_prices(Rates().begin(), Rates().begin() + steps);
  for (double& b : bond_prices) b = 1 - b / 4;  // 3-month bonds.
  for (auto _ : state) {
    benchmark::DoNotOptimize(MoneyMarketRollover(100.0, bond_prices));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MoneyMarketRollover)->Arg(4)->Arg(120)->Arg(1024);

}  // namespace
}  // namespace m4f::ch02
