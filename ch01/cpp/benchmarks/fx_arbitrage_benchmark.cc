// Microbenchmarks for the currency-cycle search (m4f/fx_arbitrage.h).
//
// FxMarket::BestCycle tries every simple cycle through the start currency,
// so its cost grows roughly like (n-1)! in the number of currencies n. The
// book's Exercise 1.3 has three currencies; BM_BestCycleScaling shows how
// fast that changes.
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/fx_arbitrage.h"

namespace m4f::ch01 {
namespace {

// Exercise 1.3: dealers A (New York) and B (London).
std::vector<FxQuote> BookQuotes() {
  return {{"A", "EUR", "USD", 1.0202, 1.0284},
          {"A", "GBP", "USD", 1.5718, 1.5844},
          {"B", "EUR", "GBP", 0.6324, 0.6401},
          {"B", "USD", "GBP", 0.6299, 0.6375}};
}

// A complete market on `n` currencies "C0".."C<n-1>": one dealer quotes every
// pair, around consistent mid rates with a 0.5% spread plus some noise, so a
// few cycles are profitable.
std::vector<FxQuote> SyntheticQuotes(int n) {
  std::mt19937_64 rng(static_cast<std::uint64_t>(n));
  std::uniform_real_distribution<double> level(0.5, 2.0);
  std::uniform_real_distribution<double> noise(-0.004, 0.004);
  std::vector<double> mid(n);
  for (double& m : mid) m = level(rng);
  std::vector<FxQuote> quotes;
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      const double rate = mid[i] / mid[j] * (1 + noise(rng));
      quotes.push_back({"D", "C" + std::to_string(i), "C" + std::to_string(j),
                        rate * 0.9975, rate * 1.0025});
    }
  }
  return quotes;
}

void BM_FxMarketConstruct(benchmark::State& state) {
  const std::vector<FxQuote> quotes = BookQuotes();
  for (auto _ : state) {
    FxMarket market(quotes);
    benchmark::DoNotOptimize(market);
  }
}
BENCHMARK(BM_FxMarketConstruct);

void BM_BestLeg(benchmark::State& state) {
  const FxMarket market(BookQuotes());
  const std::string from = "USD";
  const std::string to = "GBP";
  for (auto _ : state) {
    benchmark::DoNotOptimize(market.BestLeg(from, to));
  }
}
BENCHMARK(BM_BestLeg);

// The whole of Exercise 1.3.
void BM_BestCycleBook(benchmark::State& state) {
  const FxMarket market(BookQuotes());
  for (auto _ : state) {
    benchmark::DoNotOptimize(market.BestCycle("USD"));
  }
}
BENCHMARK(BM_BestCycleBook);

void BM_BestCycleScaling(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  const FxMarket market(SyntheticQuotes(n));
  for (auto _ : state) {
    benchmark::DoNotOptimize(market.BestCycle("C0"));
  }
  state.SetLabel(std::to_string(n) + " currencies");
}
BENCHMARK(BM_BestCycleScaling)->DenseRange(3, 8)->Unit(benchmark::kMicrosecond);

}  // namespace
}  // namespace m4f::ch01
