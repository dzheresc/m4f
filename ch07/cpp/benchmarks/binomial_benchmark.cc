// Microbenchmarks for CRR binomial option pricing (m4f/binomial.h): European
// and American calls and puts at 10 to 2000 steps. Backward induction is
// O(N^2), so the step counts are spread out to show the fit.
#include "benchmark/benchmark.h"
#include "m4f/binomial.h"

namespace m4f::ch07 {
namespace {

void Steps(benchmark::internal::Benchmark* b) {
  for (int n : {10, 100, 500, 2000}) b->Arg(n);
  b->Complexity(benchmark::oNSquared)->Unit(benchmark::kMicrosecond);
}

// At-the-money, one year, r = 5%, sigma = 20%.
void Run(benchmark::State& state, bool is_call, bool american) {
  const int n = static_cast<int>(state.range(0));
  double strike = 100.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(strike);  // Keeps the call from being hoisted.
    benchmark::DoNotOptimize(
        CrrPrice(100.0, strike, 0.05, 0.2, 1.0, n, is_call, american));
  }
  state.SetComplexityN(n);
}

void BM_CrrEuropeanCall(benchmark::State& state) { Run(state, true, false); }
BENCHMARK(BM_CrrEuropeanCall)->Apply(Steps);

void BM_CrrEuropeanPut(benchmark::State& state) { Run(state, false, false); }
BENCHMARK(BM_CrrEuropeanPut)->Apply(Steps);

void BM_CrrAmericanCall(benchmark::State& state) { Run(state, true, true); }
BENCHMARK(BM_CrrAmericanCall)->Apply(Steps);

void BM_CrrAmericanPut(benchmark::State& state) { Run(state, false, true); }
BENCHMARK(BM_CrrAmericanPut)->Apply(Steps);

}  // namespace
}  // namespace m4f::ch07
