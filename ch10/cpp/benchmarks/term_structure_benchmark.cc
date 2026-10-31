// Microbenchmarks for the term structure: forward rates, prices from forward
// rates, coupon bond pricing off a zero curve, yield to maturity and the
// money market account (m4f/term_structure.h).
#include <cmath>
#include <cstddef>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/term_structure.h"

namespace m4f::ch10 {
namespace {

// A smooth upward-sloping zero curve at n annual maturities.
struct Curve {
  std::vector<double> maturities;
  std::vector<double> prices;
};

Curve MakeCurve(std::size_t n) {
  Curve c;
  for (std::size_t i = 1; i <= n; ++i) {
    const double t = static_cast<double>(i);
    c.maturities.push_back(t);
    c.prices.push_back(std::exp(-(0.03 + 0.02 * (1 - std::exp(-t / 5))) * t));
  }
  return c;
}

void Maturities(benchmark::internal::Benchmark* b) {
  for (int n : {3, 30, 360}) b->Arg(n);
}

void BM_ForwardRates(benchmark::State& state) {
  const Curve c = MakeCurve(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(ForwardRates(c.maturities, c.prices));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ForwardRates)->Apply(Maturities);

void BM_PricesFromForwardRates(benchmark::State& state) {
  const Curve c = MakeCurve(static_cast<std::size_t>(state.range(0)));
  const std::vector<double> f = ForwardRates(c.maturities, c.prices);
  for (auto _ : state) {
    benchmark::DoNotOptimize(PricesFromForwardRates(c.maturities, f));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PricesFromForwardRates)->Apply(Maturities);

// A bond paying a coupon at every curve maturity: each flow is matched to
// its maturity by a linear search, so this is O(n^2).
void BM_CouponBondPrice(benchmark::State& state) {
  const Curve c = MakeCurve(static_cast<std::size_t>(state.range(0)));
  std::vector<double> amounts(c.maturities.size(), 5.0);
  amounts.back() += 100.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        CouponBondPrice(c.maturities, amounts, c.maturities, c.prices));
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CouponBondPrice)->Apply(Maturities)->Complexity();

void BM_YieldToMaturity(benchmark::State& state) {
  const Curve c = MakeCurve(static_cast<std::size_t>(state.range(0)));
  std::vector<double> amounts(c.maturities.size(), 5.0);
  amounts.back() += 100.0;
  const double price =
      CouponBondPrice(c.maturities, amounts, c.maturities, c.prices);
  for (auto _ : state) {
    benchmark::DoNotOptimize(YieldToMaturity(price, c.maturities, amounts));
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_YieldToMaturity)->Apply(Maturities)->Complexity(benchmark::oN);

void BM_MoneyMarketAccount(benchmark::State& state) {
  const Curve c = MakeCurve(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(MoneyMarketAccount(c.maturities, c.prices));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MoneyMarketAccount)->Apply(Maturities);

}  // namespace
}  // namespace m4f::ch10
