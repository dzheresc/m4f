// Microbenchmarks for bond prices, duration, immunisation and dynamic
// hedging under a flat continuously compounded yield (m4f/bonds.h).
#include <cmath>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/bonds.h"

namespace m4f::ch10 {
namespace {

// Coupon bonds with 3 annual flows (the book's size), 10 annual, 60
// semi-annual (30 years) and 360 monthly (30 years).
Bond MakeBond(int n) {
  const double dt = n <= 10 ? 1.0 : (n <= 60 ? 0.5 : 1.0 / 12);
  return CouponBond(100.0, 8.0 * dt, n, dt);
}

void Flows(benchmark::internal::Benchmark* b) {
  for (int n : {3, 10, 60, 360}) b->Arg(n);
  b->Complexity(benchmark::oN);
}

void BM_Price(benchmark::State& state) {
  const Bond bond = MakeBond(static_cast<int>(state.range(0)));
  double y = 0.06;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(Price(bond, y));
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Price)->Apply(Flows);

void BM_Duration(benchmark::State& state) {
  const Bond bond = MakeBond(static_cast<int>(state.range(0)));
  double y = 0.06;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(Duration(bond, y));
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Duration)->Apply(Flows);

void BM_LogReturn(benchmark::State& state) {
  const Bond bond = MakeBond(static_cast<int>(state.range(0)));
  double y = 0.07;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(LogReturn(bond, 0.06, y, 1.5));
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_LogReturn)->Apply(Flows);

void BM_Immunise(benchmark::State& state) {
  const Bond b1 = CouponBond(100.0, 8.0, 2);
  const Bond b2 = CouponBond(100.0, 6.0, 10);
  double y = 0.06;
  for (auto _ : state) {
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(Immunise(1000.0, b1, b2, y, 0.0, 5.0));
  }
}
BENCHMARK(BM_Immunise);

// Rebalancing an immunised portfolio at 5, 52 (weekly) and 260 (daily) dates
// over a 5-year horizon along a random yield path.
void BM_DynamicHedge(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  const Bond b1 = CouponBond(100.0, 8.0, 2);
  const Bond b2 = CouponBond(100.0, 6.0, 10);
  std::mt19937_64 rng(steps);
  std::normal_distribution<double> shock(0.0, 0.01 / std::sqrt(steps / 5.0));
  std::vector<std::pair<double, double>> path{{0.0, 0.06}};
  for (std::size_t i = 1; i <= steps; ++i) {
    path.emplace_back(5.0 * static_cast<double>(i) / steps,
                      path.back().second + shock(rng));
  }
  for (auto _ : state) {
    benchmark::DoNotOptimize(DynamicHedge(1000.0, b1, b2, 5.0, path));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DynamicHedge)->Arg(5)->Arg(52)->Arg(260);

}  // namespace
}  // namespace m4f::ch10
