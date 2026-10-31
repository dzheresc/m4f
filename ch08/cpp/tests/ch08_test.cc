// Unit tests for chapter 8. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "m4f/binomial_pricing.h"
#include "m4f/black_scholes.h"

namespace m4f::ch08 {
namespace {

class Checker {
 public:
  void Expect(bool ok, const char* what) {
    if (!ok) {
      std::printf("FAIL: %s\n", what);
      ++failures_;
    }
  }
  int failures() const { return failures_; }

 private:
  int failures_ = 0;
};

bool Near(double a, double b, double eps = 1e-9) {
  return std::abs(a - b) < eps;
}

Payoff Call(double strike) {
  return [strike](double s) { return std::max(s - strike, 0.0); };
}

Payoff Put(double strike) {
  return [strike](double s) { return std::max(strike - s, 0.0); };
}

// Section 8.1.1.
void TestOneStep(Checker& t) {
  const Replication rep = OneStepReplication(100, 0.2, -0.1, 0.1, 20, 0);
  t.Expect(
      Near(rep.x * 120 + rep.y * 1.1, 20) && Near(rep.x * 90 + rep.y * 1.1, 0),
      "one-step replication");
  t.Expect(Near(rep.price, 40 / 3.3), "one-step price");
}

// Section 8.1.2: the replicating position reproduces both successor values.
void TestReplication(Checker& t) {
  const Tree values = OptionTree(100, 0.2, -0.1, 0.1, 3, Call(100));
  for (int n = 0; n < 3; ++n) {
    for (int k = 0; k <= n; ++k) {
      const auto [x, y] =
          ReplicatingPosition(100, 0.2, -0.1, 0.1, values, n, k);
      for (int next_k : {k, k + 1}) {
        t.Expect(Near(x * StockPrice(100, 0.2, -0.1, n + 1, next_k) +
                          y * std::pow(1.1, n + 1),
                      values[n + 1][next_k]),
                 "replication");
      }
    }
  }
}

// Sections 8.1.3-8.1.4.
void TestFormulas(Checker& t) {
  for (int num_steps : {1, 2, 5, 8}) {
    for (double strike : {70.0, 100.0, 130.0}) {
      const double call =
          OptionTree(100, 0.2, -0.1, 0.1, num_steps, Call(strike))[0][0];
      t.Expect(
          Near(EuropeanPriceSum(100, 0.2, -0.1, 0.1, num_steps, Call(strike)),
               call),
          "sum formula");
      t.Expect(
          Near(CrrCall(100, strike, 0.2, -0.1, 0.1, num_steps).price, call),
          "CRR formula");
      const double put =
          OptionTree(100, 0.2, -0.1, 0.1, num_steps, Put(strike))[0][0];
      t.Expect(Near(call - put, 100 - strike * std::pow(1.1, -num_steps)),
               "put-call parity");
    }
  }
  t.Expect(Near(BinomialSum(0.3, 6, 0), 1), "binomial sum");
  // Large N: C(2000, 1000) alone overflows a double.
  constexpr int kBig = 2000;
  const double tau = 1.0 / kBig;
  const double u = std::exp(0.2 * std::sqrt(tau)) - 1;
  const double d = std::exp(-0.2 * std::sqrt(tau)) - 1;
  const double r = std::exp(0.05 * tau) - 1;
  t.Expect(Near(BinomialSum(0.5, kBig, 0), 1), "large-N binomial sum");
  const double crr = CrrCall(100, 100, u, d, r, kBig).price;
  t.Expect(Near(crr, EuropeanPriceSum(100, u, d, r, kBig, Call(100)), 1e-9),
           "large-N CRR formula = sum");
  t.Expect(Near(crr, BsCall(100, 100, 0.05, 0.2, 1), 1e-2),
           "large-N CRR formula near Black-Scholes");
}

// Section 8.2.
void TestAmerican(Checker& t) {
  const Tree american = OptionTree(100, 0.1, -0.1, 0.02, 4, Put(100), true);
  const Tree european = OptionTree(100, 0.1, -0.1, 0.02, 4, Put(100));
  t.Expect(american[0][0] > european[0][0], "P_A > P_E");
  for (int n = 0; n < 5; ++n) {
    for (int k = 0; k <= n; ++k) {
      t.Expect(
          american[n][k] >=
              std::max(100 - StockPrice(100, 0.1, -0.1, n, k), 0.0) - 1e-12,
          "P_A >= payoff");
    }
  }
  t.Expect(EarlyExerciseNodes(100, 0.1, -0.1, 0.02, 4, Call(100)).empty(),
           "never exercise a call early");
  t.Expect(Near(OptionTree(100, 0.1, -0.1, 0.02, 4, Call(100), true)[0][0],
                OptionTree(100, 0.1, -0.1, 0.02, 4, Call(100))[0][0]),
           "C_A = C_E");
}

// Section 8.3.
void TestBlackScholes(Checker& t) {
  t.Expect(Near(NormCdf(0), 0.5, 1e-14), "N(0)");
  t.Expect(Near(NormCdf(1.959963984540054), 0.975, 1e-12), "N(1.96)");
  t.Expect(Near(NormCdf(-8) + NormCdf(8), 1, 1e-14), "N symmetry");
  const double call = BsCall(100, 100, 0.05, 0.2, 1);
  const double put = BsPut(100, 100, 0.05, 0.2, 1);
  t.Expect(Near(call, 10.450583572185565, 1e-10), "BS call");
  t.Expect(Near(call - put, 100 - 100 * std::exp(-0.05)), "BS parity");
  const double h = 1e-4;
  const double finite_diff = (BsCall(100 + h, 100, 0.05, 0.2, 1) -
                              BsCall(100 - h, 100, 0.05, 0.2, 1)) /
                             (2 * h);
  t.Expect(Near(BsCallDelta(100, 100, 0.05, 0.2, 1), finite_diff, 1e-6),
           "delta");
  t.Expect(Near(CrrTreePrice(100, 100, 0.05, 0.2, 1, 2000, true), call, 1e-2),
           "CRR converges to BS");
}

int RunAllTests() {
  Checker t;
  TestOneStep(t);
  TestReplication(t);
  TestFormulas(t);
  TestAmerican(t);
  TestBlackScholes(t);
  if (t.failures() == 0) std::printf("All ch08 tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch08

int main() { return m4f::ch08::RunAllTests(); }
