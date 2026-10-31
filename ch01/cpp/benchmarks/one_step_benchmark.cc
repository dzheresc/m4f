// Microbenchmarks for the one-step binomial market (m4f/one_step.h) and the
// forward-contract helpers (m4f/forward.h).
//
// Every call here is a handful of floating-point operations, so each
// benchmark cycles through a table of distinct markets. That keeps the
// compiler from folding the work into a constant and keeps the inputs out of
// registers, which is closer to how the functions are used in a pricing loop.
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/forward.h"
#include "m4f/one_step.h"

namespace m4f::ch01 {
namespace {

constexpr std::size_t kNumMarkets = 1024;  // Power of two: cheap wrap-around.

struct MarketInputs {
  double a0, a1, s0, s_up, s_down, p;
};

// Random but reproducible markets around the book's examples
// (A(0)=100, S(0)=100); about half admit arbitrage.
const std::vector<MarketInputs>& Inputs() {
  static const std::vector<MarketInputs> inputs = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> rate(0.0, 0.15);
    std::uniform_real_distribution<double> up(0.0, 0.5);
    std::uniform_real_distribution<double> down(-0.4, 0.1);
    std::uniform_real_distribution<double> prob(0.05, 0.95);
    std::vector<MarketInputs> v;
    v.reserve(kNumMarkets);
    for (std::size_t i = 0; i < kNumMarkets; ++i) {
      const double d = down(rng);
      const double u = d + 0.01 + up(rng);
      v.push_back({100.0, 100.0 * (1 + rate(rng)), 100.0, 100.0 * (1 + u),
                   100.0 * (1 + d), prob(rng)});
    }
    return v;
  }();
  return inputs;
}

const std::vector<OneStepMarket>& Markets() {
  static const std::vector<OneStepMarket> markets = [] {
    std::vector<OneStepMarket> v;
    v.reserve(kNumMarkets);
    for (const MarketInputs& in : Inputs()) {
      v.emplace_back(in.a0, in.a1, in.s0, in.s_up, in.s_down, in.p);
    }
    return v;
  }();
  return markets;
}

// Runs `op(market)` once per iteration, cycling through Markets().
template <typename Op>
void RunOverMarkets(benchmark::State& state, Op op) {
  const std::vector<OneStepMarket>& markets = Markets();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(markets[i]));
    i = (i + 1) & (kNumMarkets - 1);
  }
  state.SetItemsProcessed(state.iterations());
}

void BM_Construct(benchmark::State& state) {
  const std::vector<MarketInputs>& inputs = Inputs();
  std::size_t i = 0;
  for (auto _ : state) {
    const MarketInputs& in = inputs[i];
    OneStepMarket market(in.a0, in.a1, in.s0, in.s_up, in.s_down, in.p);
    benchmark::DoNotOptimize(market);
    i = (i + 1) & (kNumMarkets - 1);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Construct);

void BM_IsArbitrageFree(benchmark::State& state) {
  RunOverMarkets(state,
                 [](const OneStepMarket& m) { return m.IsArbitrageFree(); });
}
BENCHMARK(BM_IsArbitrageFree);

void BM_Arbitrage(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) { return m.Arbitrage(); });
}
BENCHMARK(BM_Arbitrage);

void BM_RiskNeutralProb(benchmark::State& state) {
  RunOverMarkets(state,
                 [](const OneStepMarket& m) { return m.RiskNeutralProb(); });
}
BENCHMARK(BM_RiskNeutralProb);

void BM_Replicate(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) {
    return m.Replicate(CallPayoff(m.s1(State::kUp), 100.0),
                       CallPayoff(m.s1(State::kDown), 100.0));
  });
}
BENCHMARK(BM_Replicate);

// Call price two ways: replicating portfolio vs. risk-neutral expectation.
void BM_PriceByReplication(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) {
    return m.Price(CallPayoff(m.s1(State::kUp), 100.0),
                   CallPayoff(m.s1(State::kDown), 100.0));
  });
}
BENCHMARK(BM_PriceByReplication);

void BM_RiskNeutralPrice(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) {
    return m.RiskNeutralPrice(CallPayoff(m.s1(State::kUp), 100.0),
                              CallPayoff(m.s1(State::kDown), 100.0));
  });
}
BENCHMARK(BM_RiskNeutralPrice);

// Expected return and risk of a 50/50 portfolio (Section 1.4).
void BM_PortfolioRisk(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) {
    const Portfolio portfolio = m.Split(10000.0, 0.5);
    const double k_up = m.PortfolioReturn(portfolio, State::kUp);
    const double k_down = m.PortfolioReturn(portfolio, State::kDown);
    return m.Expect(k_up, k_down) + m.StdDev(k_up, k_down);
  });
}
BENCHMARK(BM_PortfolioRisk);

void BM_HedgedPurchaseCost(benchmark::State& state) {
  RunOverMarkets(state, [](const OneStepMarket& m) {
    return HedgedPurchaseCost(m, 100.0, 2.0, State::kUp);
  });
}
BENCHMARK(BM_HedgedPurchaseCost);

void BM_ForwardArbitrage(benchmark::State& state) {
  const std::vector<MarketInputs>& inputs = Inputs();
  std::size_t i = 0;
  for (auto _ : state) {
    const MarketInputs& in = inputs[i];
    // Quote the forward at S^u(1): mispriced in every generated market.
    benchmark::DoNotOptimize(ForwardArbitrage(in.s0, in.a0, in.a1, in.s_up));
    i = (i + 1) & (kNumMarkets - 1);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ForwardArbitrage);

}  // namespace
}  // namespace m4f::ch01
