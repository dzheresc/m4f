// Unit tests for chapter 11. A tiny self-contained harness keeps the build
// free of third-party dependencies; each check mirrors a Kotlin and Python
// test.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "m4f/derivatives.h"
#include "m4f/rate_tree.h"

namespace m4f::ch11 {
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

constexpr int kNumSteps = 4;
constexpr double kTau = 1.0;

struct Tree {
  Lattice rates = RateLattice(0.05, 0.01, -0.01, kNumSteps);
  Lattice probs = ConstantProbs(0.5, kNumSteps);
};

// Section 11.1.
void TestRateTree(Checker& t, const Tree& tree) {
  t.Expect(tree.rates.size() == 5 && Near(tree.rates[2][2], 0.07) &&
               Near(tree.rates[2][0], 0.03),
           "rate lattice");
  const std::vector<double> account = MoneyMarket(tree.rates, kTau, "ud");
  t.Expect(Near(account[1], std::exp(0.05)) &&
               Near(account[2], std::exp(0.05 + 0.06)),
           "money market");
  t.Expect(Near(OneStepPrice(0.05, 0.5), std::exp(-0.025)), "one-step price");
}

// Section 11.2.
void TestBondPricing(Checker& t, const Tree& tree) {
  const Lattice& rates = tree.rates;
  const Lattice& probs = tree.probs;
  const Lattice b1 = BondLattice(rates, probs, kTau, 1);
  t.Expect(Near(b1[0][0], std::exp(-0.05)), "B(0,1)");
  const Lattice b2 = BondLattice(rates, probs, kTau, 2);
  t.Expect(Near(b2[0][0], std::exp(-0.05) *
                              (0.5 * std::exp(-0.06) + 0.5 * std::exp(-0.04))),
           "B(0,2)");
  const std::vector<double> b0 = BondPrices(rates, probs, kTau, kNumSteps);
  t.Expect(b0.size() == 4, "4 prices");
  for (int m = 1; m <= kNumSteps; ++m) {
    t.Expect(Near(b0[m - 1], BondLattice(rates, probs, kTau, m)[0][0]),
             "bond prices");
  }
  for (std::size_t i = 1; i < b0.size(); ++i) {
    t.Expect(b0[i - 1] > b0[i], "decreasing in maturity");
  }
  t.Expect(Near(YieldRate(b1[0][0], 0, 1, kTau), 0.05), "yield");
  const Lattice b4 = BondLattice(rates, probs, kTau, kNumSteps);
  for (int n = 0; n < kNumSteps; ++n) {
    for (int k = 0; k <= n; ++k) {
      t.Expect(Near(b4[n][k] * std::exp(rates[n][k] * kTau),
                    0.5 * b4[n + 1][k + 1] + 0.5 * b4[n + 1][k]),
               "martingale");
    }
  }
}

// Section 11.2.1.
void TestImpliedProbs(Checker& t, const Tree& tree) {
  for (double p : {0.5, 0.6, 0.35}) {
    const Lattice bond =
        BondLattice(tree.rates, ConstantProbs(p, kNumSteps), kTau, kNumSteps);
    for (const auto& row : ImpliedProbs(bond, tree.rates, kTau)) {
      for (double q : row) t.Expect(Near(q, p), "implied p*");
    }
  }
  const Lattice b2 = BondLattice(tree.rates, tree.probs, kTau, 2);
  t.Expect(Near(ImpliedProb(b2, tree.rates, kTau, 0, 0), 0.5),
           "implied p* from B2");
}

// Section 11.3.1.
void TestBondOptions(Checker& t, const Tree& tree) {
  const Lattice& rates = tree.rates;
  const Lattice& probs = tree.probs;
  const Lattice b2 = BondLattice(rates, probs, kTau, 2);
  const Lattice b4 = BondLattice(rates, probs, kTau, kNumSteps);
  const Lattice call = BondOption(rates, probs, kTau, kNumSteps, 2, 0.9, true);
  const Lattice put = BondOption(rates, probs, kTau, kNumSteps, 2, 0.9, false);
  t.Expect(Near(call[0][0] - put[0][0], b4[0][0] - 0.9 * b2[0][0]),
           "put-call parity");
  const Lattice american_put =
      BondOption(rates, probs, kTau, kNumSteps, 2, 0.9, false, true);
  t.Expect(american_put[0][0] >= put[0][0] &&
               Near(american_put[0][0], 0.9 - b4[0][0]),
           "American put");
  t.Expect(Near(BondOption(rates, probs, kTau, kNumSteps, 2, 0.0, true)[0][0],
                b4[0][0]),
           "zero-strike call = bond");
}

// Sections 11.3.2-11.3.3.
void TestSwapsCapsFloors(Checker& t, const Tree& tree) {
  const Lattice& rates = tree.rates;
  const Lattice& probs = tree.probs;
  const std::vector<double> b0 = BondPrices(rates, probs, kTau, kNumSteps);
  const double swap_rate = SwapRate(b0);
  t.Expect(Near(PayerSwap(rates, probs, kTau, kNumSteps, swap_rate)[0][0], 0.0),
           "swap worthless at swap rate");
  t.Expect(
      Near(PayerSwap(rates, probs, kTau, kNumSteps, 0.0)[0][0], 1 - b0.back()),
      "floating leg = 1 - B(0,N)");
  for (double strike : {0.03, swap_rate, 0.07}) {
    const double cap = Cap(rates, probs, kTau, kNumSteps, strike)[0][0];
    const double floor = Floor(rates, probs, kTau, kNumSteps, strike)[0][0];
    t.Expect(Near(cap - floor,
                  PayerSwap(rates, probs, kTau, kNumSteps, strike)[0][0]),
             "cap - floor = swap");
    t.Expect(cap >= 0.0 && floor >= 0.0, "nonnegative");
  }
  double sum = 0.0;
  for (int n = 0; n < kNumSteps; ++n) {
    sum += Caplet(rates, probs, kTau, n, 0.05);
  }
  t.Expect(Near(sum, Cap(rates, probs, kTau, kNumSteps, 0.05)[0][0]),
           "caplets sum to cap");
  t.Expect(Near(Caplet(rates, probs, kTau, 0, 0.05),
                std::exp(-0.05) *
                    std::max(FloatingRate(rates, kTau, 0, 0) - 0.05, 0.0)),
           "first caplet");

  const Lattice annuity =
      DiscountBack(rates, probs, kTau, std::vector<double>(kNumSteps + 1, 0.0),
                   [](int, int) { return 1.0; });
  double sum_bonds = 0.0;
  for (double b : b0) sum_bonds += b;
  t.Expect(Near(annuity[0][0], sum_bonds), "annuity = sum of bonds");
}

int RunAllTests() {
  Checker t;
  const Tree tree;
  TestRateTree(t, tree);
  TestBondPricing(t, tree);
  TestImpliedProbs(t, tree);
  TestBondOptions(t, tree);
  TestSwapsCapsFloors(t, tree);
  if (t.failures() == 0) std::printf("All C++ tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch11

int main() { return m4f::ch11::RunAllTests(); }
