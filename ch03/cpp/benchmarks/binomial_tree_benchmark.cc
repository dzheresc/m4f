// Microbenchmarks for the binomial tree model (m4f/binomial_tree.h).
//
// Price, DiscountedPrice and BinomialProb call std::pow, and ExpectedPrice
// sums BinomialProb over all n+1 nodes, so the step-dependent ones run at
// several tree depths.
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/binomial_tree.h"

namespace m4f::ch03 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

// Reproducible arbitrage-free trees around S(0) = 100.
const std::vector<BinomialTree>& Trees() {
  static const std::vector<BinomialTree> trees = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> rate(0.0, 0.05);
    std::uniform_real_distribution<double> spread(0.01, 0.2);
    std::vector<BinomialTree> v;
    v.reserve(kNumInputs);
    for (std::size_t i = 0; i < kNumInputs; ++i) {
      const double r = rate(rng);
      v.push_back({100.0, r + spread(rng), r - spread(rng), r});
    }
    return v;
  }();
  return trees;
}

// Runs `op(tree)` once per iteration, cycling through Trees().
template <typename Op>
void RunOverTrees(benchmark::State& state, Op op) {
  const std::vector<BinomialTree>& trees = Trees();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(trees[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
  state.SetItemsProcessed(state.iterations());
}

// Tree depths: the book's 3 steps, then 10, 100 and 1000.
void Depths(benchmark::internal::Benchmark* b) {
  for (int n : {3, 10, 100, 1000}) b->Arg(n);
}

void BM_BinomialProb(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  RunOverTrees(state, [n](const BinomialTree& t) {
    return BinomialProb(n, n / 2, t.RiskNeutralProb());
  });
}
BENCHMARK(BM_BinomialProb)->Apply(Depths);

void BM_Price(benchmark::State& state) {
  RunOverTrees(state, [](const BinomialTree& t) { return t.Price(10, 4); });
}
BENCHMARK(BM_Price);

void BM_RiskNeutralProb(benchmark::State& state) {
  RunOverTrees(state,
               [](const BinomialTree& t) { return t.RiskNeutralProb(); });
}
BENCHMARK(BM_RiskNeutralProb);

void BM_ExpectedPrice(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  RunOverTrees(state,
               [n](const BinomialTree& t) { return t.ExpectedPrice(n, 0.6); });
  state.SetComplexityN(n);
}
BENCHMARK(BM_ExpectedPrice)->Apply(Depths)->Complexity();

void BM_ConditionalDiscounted(benchmark::State& state) {
  RunOverTrees(state, [](const BinomialTree& t) {
    return t.ConditionalDiscounted(10, 4);
  });
}
BENCHMARK(BM_ConditionalDiscounted);

}  // namespace
}  // namespace m4f::ch03
