#include "m4f/report.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "m4f/derivatives.h"
#include "m4f/rate_tree.h"

namespace m4f::ch11 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

// "[a, b, c]" with each entry formatted by Fmt().
std::string FmtList(const std::vector<double>& values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out += (i ? ", " : "") + Fmt(values[i]);
  }
  return out + "]";
}

// Lattice rows are stored k = 0..n (down to up); the report lists them from
// the top node down.
std::vector<double> Reversed(std::vector<double> v) {
  std::reverse(v.begin(), v.end());
  return v;
}

// The tree of Section 11.1: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau = 1.
constexpr int kNumSteps = 4;
constexpr double kTau = 1.0;

struct Tree {
  Lattice rates = RateLattice(0.05, 0.01, -0.01, kNumSteps);
  Lattice probs = ConstantProbs(0.5, kNumSteps);
};

void PrintBinomialTree(std::ostream& os, const Tree& tree) {
  os << "\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), "
        "tau=1, N=4 ==\n";
  for (int n = 0; n <= kNumSteps; ++n) {
    std::vector<double> one_step;
    for (double r : Reversed(tree.rates[n])) {
      one_step.push_back(OneStepPrice(r, kTau));
    }
    os << std::format("n={}: r={} B(n,n+1)={}\n", n,
                      FmtList(Reversed(tree.rates[n])), FmtList(one_step));
  }
  for (const char* path : {"uuuu", "udud", "dddd"}) {
    os << std::format("Money market account along {}: A={}\n", path,
                      FmtList(MoneyMarket(tree.rates, kTau, path)));
  }
}

void PrintBondPricing(std::ostream& os, const Tree& tree) {
  os << "\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==\n";
  const std::vector<double> b0 =
      BondPrices(tree.rates, tree.probs, kTau, kNumSteps);
  os << "B(0,N) for N=1..4: " << FmtList(b0) << "\n";
  std::vector<double> yields;
  for (int m = 0; m < kNumSteps; ++m) {
    yields.push_back(YieldRate(b0[m], 0, m + 1, kTau));
  }
  os << "Term structure y(0,N): " << FmtList(yields) << "\n";
  const Lattice b4 = BondLattice(tree.rates, tree.probs, kTau, kNumSteps);
  for (int n = 0; n <= kNumSteps; ++n) {
    os << std::format("B(n,4) at n={}: {}\n", n, FmtList(Reversed(b4[n])));
  }
  const Lattice b3 = BondLattice(tree.rates, tree.probs, kTau, 3);
  const Lattice b2 = BondLattice(tree.rates, tree.probs, kTau, 2);
  const std::vector<std::pair<int, const Lattice*>> lattices = {
      {2, &b2}, {3, &b3}, {4, &b4}};
  std::vector<double> y_up;
  std::vector<double> y_down;
  std::vector<double> returns;
  for (const auto& [m, bond] : lattices) {
    y_up.push_back(YieldRate((*bond)[1][1], 1, m, kTau));
    y_down.push_back(YieldRate((*bond)[1][0], 1, m, kTau));
    returns.push_back((0.5 * (*bond)[1][1] + 0.5 * (*bond)[1][0]) /
                      (*bond)[0][0]);
  }
  os << "Term structure after one step: up node y(1,N)=" << FmtList(y_up)
     << " down node y(1,N)=" << FmtList(y_down) << "\n";
  os << "Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: "
     << FmtList(returns)
     << std::format(" = e^(r(0) tau) = {}\n", Fmt(std::exp(0.05 * kTau)));

  os << "\n== 11.2.1 Risk-neutral probabilities implied by bond prices ==\n";
  for (const auto& [m, bond] :
       {std::pair<int, const Lattice*>{4, &b4}, {3, &b3}, {2, &b2}}) {
    const Lattice implied = ImpliedProbs(*bond, tree.rates, kTau);
    std::string row = std::format("from B(n,{}):", m);
    for (std::size_t n = 0; n < implied.size(); ++n) {
      row += std::format(" n={}:{}", n, FmtList(Reversed(implied[n])));
    }
    os << row << "\n";
  }
  const Lattice probs6 = ConstantProbs(0.6, kNumSteps);
  const Lattice b4_p6 = BondLattice(tree.rates, probs6, kTau, kNumSteps);
  os << std::format(
      "Bonds priced with p*=0.6: B(0,4)={} (vs {}); implied p* at (0,0): {}\n",
      Fmt(b4_p6[0][0]), Fmt(b4[0][0]),
      Fmt(ImpliedProb(b4_p6, tree.rates, kTau, 0, 0)));
  Lattice wrong = b2;
  wrong[0][0] = b2[0][0] - 0.01;
  os << std::format(
      "A two-year bond quoted at {} instead of {} implies p*={} at (0,0), "
      "inconsistent with the other bonds: arbitrage\n",
      Fmt(wrong[0][0]), Fmt(b2[0][0]),
      Fmt(ImpliedProb(wrong, tree.rates, kTau, 0, 0)));
}

void PrintBondOptions(std::ostream& os, const Tree& tree) {
  os << "\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==\n";
  const Lattice& rates = tree.rates;
  const Lattice& probs = tree.probs;
  const double strike = 0.90;
  for (bool is_call : {true, false}) {
    const Lattice v =
        BondOption(rates, probs, kTau, kNumSteps, 2, strike, is_call);
    os << std::format("European {} X={}: payoff at n=2 {}, price {}\n",
                      is_call ? "call" : "put", Fmt(strike),
                      FmtList(Reversed(v[2])), Fmt(v[0][0]));
  }
  const Lattice american =
      BondOption(rates, probs, kTau, kNumSteps, 2, strike, false, true);
  const Lattice b4 = BondLattice(rates, probs, kTau, kNumSteps);
  std::vector<double> exercise;
  for (double b : Reversed(b4[1]))
    exercise.push_back(std::max(strike - b, 0.0));
  os << std::format(
      "American put X={}: price {} = immediate exercise X - B(0,4) = {}; "
      "values at n=1 {} = exercise values {}\n",
      Fmt(strike), Fmt(american[0][0]), Fmt(strike - b4[0][0]),
      FmtList(Reversed(american[1])), FmtList(exercise));
  const double american_call =
      BondOption(rates, probs, kTau, kNumSteps, 3, 0.85, true, true)[0][0];
  const double european_call =
      BondOption(rates, probs, kTau, kNumSteps, 3, 0.85, true)[0][0];
  os << std::format(
      "American call X=0.85 on the 4-year bond, expiry n=3: {} vs European "
      "{}\n",
      Fmt(american_call), Fmt(european_call));
  os << "Call on the 2-year bond expiring at n=1, X=0.95: "
     << Fmt(BondOption(rates, probs, kTau, 2, 1, 0.95, true)[0][0]) << "\n";
}

void PrintSwapsCapsFloors(std::ostream& os, const Tree& tree) {
  const Lattice& rates = tree.rates;
  const Lattice& probs = tree.probs;
  os << "\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, "
        "4 annual payments ==\n";
  for (int n = 0; n < kNumSteps; ++n) {
    std::vector<double> floating;
    for (int k = n; k >= 0; --k) {
      floating.push_back(FloatingRate(rates, kTau, n, k));
    }
    os << std::format("n={}: L={}\n", n, FmtList(floating));
  }
  const double swap_rate = SwapRate(BondPrices(rates, probs, kTau, kNumSteps));
  os << std::format("Swap rate K = (1 - B(0,4)) / sum B(0,n) = {}\n",
                    Fmt(swap_rate));
  const Lattice swap = PayerSwap(rates, probs, kTau, kNumSteps, swap_rate);
  os << std::format("Payer swap value at K: {}; at n=1: {}\n", Fmt(swap[0][0]),
                    FmtList(Reversed(swap[1])));
  for (double fixed_rate : {0.04, 0.05, 0.06}) {
    os << std::format(
        "Payer swap at K={}: {}\n", Fmt(fixed_rate),
        Fmt(PayerSwap(rates, probs, kTau, kNumSteps, fixed_rate)[0][0]));
  }

  os << "\n== 11.3.3 Caps and floors on the same floating rate ==\n";
  for (double strike : {0.04, swap_rate, 0.06}) {
    const double cap = Cap(rates, probs, kTau, kNumSteps, strike)[0][0];
    const double floor = Floor(rates, probs, kTau, kNumSteps, strike)[0][0];
    const double swap_value =
        PayerSwap(rates, probs, kTau, kNumSteps, strike)[0][0];
    os << std::format("K={}: cap={} floor={} cap-floor={} = payer swap {}\n",
                      Fmt(strike), Fmt(cap), Fmt(floor), Fmt(cap - floor),
                      Fmt(swap_value));
  }
  std::vector<double> caplets;
  double sum = 0.0;
  for (int n = 0; n < kNumSteps; ++n) {
    caplets.push_back(Caplet(rates, probs, kTau, n, 0.05));
    sum += caplets.back();
  }
  os << "Caplets at K=0.05 by fixing date n=0..3: " << FmtList(caplets)
     << std::format(", sum {} = cap {}\n", Fmt(sum),
                    Fmt(Cap(rates, probs, kTau, kNumSteps, 0.05)[0][0]));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 11: Stochastic Interest Rates (illustrative numbers, book "
        "formulas)\n";
  const Tree tree;
  PrintBinomialTree(os, tree);
  PrintBondPricing(os, tree);
  PrintBondOptions(os, tree);
  PrintSwapsCapsFloors(os, tree);
}

}  // namespace m4f::ch11
