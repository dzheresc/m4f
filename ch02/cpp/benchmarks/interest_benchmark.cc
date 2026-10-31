// Microbenchmarks for Section 2.1, time value of money (m4f/interest.h).
//
// Each function is a few arithmetic operations plus at most one pow, exp or
// log, so each benchmark cycles through a table of distinct inputs to keep the
// compiler from folding the call into a constant.
#include <cstddef>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "m4f/interest.h"

namespace m4f::ch02 {
namespace {

constexpr std::size_t kNumInputs = 1024;  // Power of two: cheap wrap-around.

struct Inputs {
  double amount;  // Principal, value, payment or loan.
  double r;       // Annual rate, 1% - 15%.
  double t;       // Years, 0.25 - 30.
  int m;          // Compounding periods per year.
  int n;          // Number of payments, 1 - 360.
};

const std::vector<Inputs>& Table() {
  static const std::vector<Inputs> table = [] {
    std::mt19937_64 rng(20261031);
    std::uniform_real_distribution<double> amount(100.0, 100000.0);
    std::uniform_real_distribution<double> rate(0.01, 0.15);
    std::uniform_real_distribution<double> years(0.25, 30.0);
    std::uniform_int_distribution<int> payments(1, 360);
    constexpr int kPeriods[] = {1, 2, 4, 12, 365};
    std::vector<Inputs> v;
    v.reserve(kNumInputs);
    for (std::size_t i = 0; i < kNumInputs; ++i) {
      v.push_back({amount(rng), rate(rng), years(rng), kPeriods[rng() % 5],
                   payments(rng)});
    }
    return v;
  }();
  return table;
}

// Runs `op(inputs)` once per iteration, cycling through Table().
template <typename Op>
void RunOverInputs(benchmark::State& state, Op op) {
  const std::vector<Inputs>& table = Table();
  std::size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(op(table[i]));
    i = (i + 1) & (kNumInputs - 1);
  }
  state.SetItemsProcessed(state.iterations());
}

void BM_SimpleValue(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return SimpleValue(in.amount, in.r, in.t);
  });
}
BENCHMARK(BM_SimpleValue);

void BM_SimpleRate(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return SimpleRate(in.amount, in.amount * (1 + in.r), in.t);
  });
}
BENCHMARK(BM_SimpleRate);

void BM_PeriodicValue(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return PeriodicValue(in.amount, in.r, in.m, in.t);
  });
}
BENCHMARK(BM_PeriodicValue);

void BM_EffectiveRate(benchmark::State& state) {
  RunOverInputs(state,
                [](const Inputs& in) { return EffectiveRate(in.r, in.m); });
}
BENCHMARK(BM_EffectiveRate);

void BM_AnnuityPresentValue(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return AnnuityPresentValue(in.amount, in.r / 12, in.n);
  });
}
BENCHMARK(BM_AnnuityPresentValue);

void BM_AnnuityFutureValue(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return AnnuityFutureValue(in.amount, in.r / 12, in.n);
  });
}
BENCHMARK(BM_AnnuityFutureValue);

void BM_LoanInstallment(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return LoanInstallment(in.amount, in.r / 12, in.n);
  });
}
BENCHMARK(BM_LoanInstallment);

void BM_ContinuousValue(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return ContinuousValue(in.amount, in.r, in.t);
  });
}
BENCHMARK(BM_ContinuousValue);

void BM_ContinuousFromPeriodic(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return ContinuousFromPeriodic(in.r, in.m);
  });
}
BENCHMARK(BM_ContinuousFromPeriodic);

void BM_PeriodicFromContinuous(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return PeriodicFromContinuous(in.r, in.m);
  });
}
BENCHMARK(BM_PeriodicFromContinuous);

void BM_PeriodicFromPeriodic(benchmark::State& state) {
  RunOverInputs(state, [](const Inputs& in) {
    return PeriodicFromPeriodic(in.r, in.m, 12);
  });
}
BENCHMARK(BM_PeriodicFromPeriodic);

}  // namespace
}  // namespace m4f::ch02
