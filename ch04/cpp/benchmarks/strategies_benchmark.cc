// Microbenchmarks for self-financing strategies along one price path
// (m4f/strategies.h), at 12 (monthly), 252 (daily) and 2520 (ten years of
// daily) steps.
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/strategies.h"

namespace m4f::ch04 {
namespace {

// Reproducible inputs for `stocks` stocks over `steps` steps: lognormal
// prices from 100, A(n) = 1.0001^n, positions in [0, 20).
struct Market {
  Mat s;
  Vec a;
  Mat x;
  Mat dividends;
};

Market MakeMarket(std::size_t stocks, std::size_t steps) {
  std::mt19937_64 rng(stocks * 100003 + steps);
  std::normal_distribution<double> shock(0.0003, 0.01);
  std::uniform_real_distribution<double> position(0.0, 20.0);
  Market m;
  for (std::size_t n = 0; n <= steps; ++n) m.a.push_back(std::pow(1.0001, n));
  for (std::size_t j = 0; j < stocks; ++j) {
    Vec s{100.0};
    Vec x;
    Vec div(steps + 1, 0.0);
    for (std::size_t n = 1; n <= steps; ++n) {
      s.push_back(s.back() * std::exp(shock(rng)));
      x.push_back(position(rng));
      if (n % 63 == 0) div[n] = 0.5;  // Quarterly dividend.
    }
    m.s.push_back(s);
    m.x.push_back(x);
    m.dividends.push_back(div);
  }
  return m;
}

void Steps(benchmark::internal::Benchmark* b) {
  for (int n : {12, 252, 2520}) b->Arg(n);
}

void BM_SelfFinancing(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  const Market m = MakeMarket(1, steps);
  for (auto _ : state) {
    benchmark::DoNotOptimize(SelfFinancing(100.0, m.s[0], m.a, m.x[0]));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SelfFinancing)->Apply(Steps);

// Section 4.2: several stocks with dividends.
void BM_SelfFinancingMulti(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  const Market m = MakeMarket(5, steps);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        SelfFinancingMulti(1000.0, m.s, m.a, m.x, &m.dividends));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SelfFinancingMulti)->Apply(Steps);

void BM_Gains(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  const Market m = MakeMarket(1, steps);
  const Vec y = SelfFinancing(100.0, m.s[0], m.a, m.x[0]).y;
  for (auto _ : state) {
    benchmark::DoNotOptimize(Gains(m.s[0], m.a, m.x[0], y));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Gains)->Apply(Steps);

void BM_DiscountedGains(benchmark::State& state) {
  const std::size_t steps = static_cast<std::size_t>(state.range(0));
  const Market m = MakeMarket(1, steps);
  for (auto _ : state) {
    benchmark::DoNotOptimize(DiscountedGains(m.s[0], m.a, m.x[0]));
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DiscountedGains)->Apply(Steps);

}  // namespace
}  // namespace m4f::ch04
