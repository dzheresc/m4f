// Microbenchmarks for the lognormal model, expected payoffs and value at
// risk of option strategies (m4f/risk.h).
#include "benchmark/benchmark.h"
#include "m4f/risk.h"

namespace m4f::ch09 {
namespace {

const Lognormal kModel{100.0, 0.12, 0.25, 0.5};

void BM_Quantile(benchmark::State& state) {
  double p = 0.05;
  for (auto _ : state) {
    benchmark::DoNotOptimize(p);
    benchmark::DoNotOptimize(Quantile(kModel, p));
  }
}
BENCHMARK(BM_Quantile);

void BM_Cdf(benchmark::State& state) {
  double s = 90.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(Cdf(kModel, s));
  }
}
BENCHMARK(BM_Cdf);

void BM_ExpectedCallPayoff(benchmark::State& state) {
  double strike = 105.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(strike);
    benchmark::DoNotOptimize(ExpectedCallPayoff(kModel, strike));
  }
}
BENCHMARK(BM_ExpectedCallPayoff);

// A bull spread mixed with stock: two option legs plus shares.
Strategy Sample() {
  const Strategy stock = AllInStock(10000.0, 100.0);
  const Strategy bull =
      BullSpread("bull", 10000.0, 100.0, 100.0, 120.0, 0.05, 0.25, 0.5);
  return Mix("mix", stock, 0.5, bull, 0.5);
}

void BM_ExpectedValue(benchmark::State& state) {
  const Strategy st = Sample();
  for (auto _ : state)
    benchmark::DoNotOptimize(ExpectedValue(st, kModel, 0.05));
}
BENCHMARK(BM_ExpectedValue);

void BM_InitialCost(benchmark::State& state) {
  const Strategy st = Sample();
  for (auto _ : state) {
    benchmark::DoNotOptimize(InitialCost(st, 100.0, 0.05, 0.25, 0.5));
  }
}
BENCHMARK(BM_InitialCost);

void BM_ValueAtRisk(benchmark::State& state) {
  const Strategy st = Sample();
  for (auto _ : state) {
    benchmark::DoNotOptimize(ValueAtRisk(st, kModel, 0.05, 0.25, 0.95));
  }
}
BENCHMARK(BM_ValueAtRisk);

void BM_ExpectedReturn(benchmark::State& state) {
  const Strategy st = Sample();
  for (auto _ : state) {
    benchmark::DoNotOptimize(ExpectedReturn(st, kModel, 0.05, 0.25));
  }
}
BENCHMARK(BM_ExpectedReturn);

}  // namespace
}  // namespace m4f::ch09
