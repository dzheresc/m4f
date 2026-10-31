// Microbenchmarks for delta and delta-Greek hedging and for rebalancing a
// delta hedge along a price path (m4f/hedging.h).
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/hedging.h"

namespace m4f::ch09 {
namespace {

// A book of n written options with strikes 80-120 and expiries up to a year.
std::vector<Position> Book(std::size_t n) {
  std::vector<Position> book;
  for (std::size_t i = 0; i < n; ++i) {
    book.push_back({-1.0 - static_cast<double>(i % 3),
                    80.0 + 40.0 * static_cast<double>(i) / n,
                    0.25 + 0.75 * static_cast<double>(i % 4) / 3, i % 2 == 0});
  }
  return book;
}

void BookSizes(benchmark::internal::Benchmark* b) {
  for (int n : {1, 10, 100}) b->Arg(n);
}

void BM_TotalGreek(benchmark::State& state) {
  const std::vector<Position> book =
      Book(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(TotalGreek(book, 100.0, 0.05, 0.2, Greek::kGamma));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_TotalGreek)->Apply(BookSizes);

void BM_DeltaHedge(benchmark::State& state) {
  const std::vector<Position> book =
      Book(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(DeltaHedge(book, 100.0, 0.05, 0.2));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DeltaHedge)->Apply(BookSizes);

void BM_DeltaGammaHedge(benchmark::State& state) {
  const std::vector<Position> book =
      Book(static_cast<std::size_t>(state.range(0)));
  const Position extra{1.0, 100.0, 0.5, true};
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        DeltaGreekHedge(book, extra, 100.0, 0.05, 0.2, Greek::kGamma));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DeltaGammaHedge)->Apply(BookSizes);

void BM_PortfolioValue(benchmark::State& state) {
  const Hedge hedge = DeltaHedge(Book(static_cast<std::size_t>(state.range(0))),
                                 100.0, 0.05, 0.2);
  for (auto _ : state) {
    benchmark::DoNotOptimize(PortfolioValue(hedge, 101.0, 0.1, 0.05, 0.2));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PortfolioValue)->Apply(BookSizes);

// Weekly (52) and daily (252) rebalancing over a year for a 10-option book.
void BM_RebalancePath(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  std::vector<Position> book = Book(10);
  for (Position& p : book) p.expiry = 1.5;  // Outlives the path.
  std::mt19937_64 rng(steps);
  std::normal_distribution<double> shock(0.0, 0.2 / std::sqrt(steps));
  std::vector<double> prices{100.0};
  for (std::size_t i = 0; i < steps; ++i) {
    prices.push_back(prices.back() * std::exp(shock(rng)));
  }
  const double dt = 1.0 / static_cast<double>(steps);
  for (auto _ : state) {
    benchmark::DoNotOptimize(RebalancePath(book, prices, dt, 0.05, 0.2));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RebalancePath)->Arg(52)->Arg(252)->Unit(benchmark::kMicrosecond);

}  // namespace
}  // namespace m4f::ch09
