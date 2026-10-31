#include "m4f/report.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "m4f/binomial_pricing.h"
#include "m4f/black_scholes.h"

namespace m4f::ch08 {
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

Payoff Call(double strike) {
  return [strike](double s) { return std::max(s - strike, 0.0); };
}

Payoff Put(double strike) {
  return [strike](double s) { return std::max(strike - s, 0.0); };
}

std::vector<double> Reversed(std::vector<double> v) {
  std::reverse(v.begin(), v.end());
  return v;
}

void PrintOneStep(std::ostream& os) {
  os << "\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 ==\n";
  const Replication rep = OneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0);
  const double q = RiskNeutralProb(0.2, -0.1, 0.1);
  os << std::format(
      "Replicate with x={} shares, y={} in the money market: C(0)={}\n",
      Fmt(rep.x), Fmt(rep.y), Fmt(rep.price));
  os << std::format("Risk-neutral: p*={}, C(0) = E*(C(1))/(1+r) = {}\n", Fmt(q),
                    Fmt((q * 20.0 + (1 - q) * 0.0) / 1.1));
}

void PrintTwoSteps(std::ostream& os) {
  os << "\n== 8.1.2 Two steps: same tree, call X=100 ==\n";
  const Tree values = OptionTree(100.0, 0.2, -0.1, 0.1, 2, Call(100.0));
  for (int n = 0; n < 3; ++n) {
    std::vector<double> s;
    for (int k = n; k >= 0; --k)
      s.push_back(StockPrice(100.0, 0.2, -0.1, n, k));
    os << std::format("n={}: S={} C={}\n", n, FmtList(s),
                      FmtList(Reversed(values[n])));
  }
  const std::vector<std::pair<int, int>> nodes{{0, 0}, {1, 1}, {1, 0}};
  for (const auto& [n, k] : nodes) {
    const auto [x, y] =
        ReplicatingPosition(100.0, 0.2, -0.1, 0.1, values, n, k);
    os << std::format("node ({},{}): x={} y={}\n", n, k, Fmt(x), Fmt(y));
  }
}

void PrintGeneralModel(std::ostream& os) {
  os << "\n== 8.1.3 General N-step model: N=5, X=100 ==\n";
  const std::vector<std::pair<const char*, Payoff>> payoffs{
      {"call", Call(100.0)}, {"put", Put(100.0)}};
  for (const auto& [name, payoff] : payoffs) {
    os << std::format("{}: backward induction={} sum formula={}\n", name,
                      Fmt(OptionTree(100.0, 0.2, -0.1, 0.1, 5, payoff)[0][0]),
                      Fmt(EuropeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, payoff)));
  }
  // Bull spread 90/120.
  const Payoff spread = [](double s) {
    return std::min(std::max(s - 90.0, 0.0), 30.0);
  };
  os << std::format("bull spread 90/120: {}\n",
                    Fmt(EuropeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, spread)));

  os << "\n== 8.1.4 Cox-Ross-Rubinstein formula ==\n";
  for (double strike : {80.0, 100.0, 150.0}) {
    const CrrCallResult c = CrrCall(100.0, strike, 0.2, -0.1, 0.1, 5);
    os << std::format(
        "N=5 X={}: m={} C={} sum formula={}\n", Fmt(strike), c.m, Fmt(c.price),
        Fmt(EuropeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, Call(strike))));
  }
}

void PrintAmerican(std::ostream& os) {
  os << "\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put "
        "X=100 ==\n";
  const Tree american = OptionTree(100.0, 0.1, -0.1, 0.02, 4, Put(100.0), true);
  const Tree european = OptionTree(100.0, 0.1, -0.1, 0.02, 4, Put(100.0));
  os << std::format("P_A={} P_E={}\n", Fmt(american[0][0]),
                    Fmt(european[0][0]));
  for (int n = 0; n < 5; ++n) {
    os << std::format("n={}: P_A={}\n", n, FmtList(Reversed(american[n])));
  }
  std::string nodes;
  for (const auto& [n, k] :
       EarlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, Put(100.0))) {
    if (!nodes.empty()) nodes += ", ";
    nodes += std::format("({},{}) S={}", n, k,
                         Fmt(StockPrice(100.0, 0.1, -0.1, n, k)));
  }
  os << "Exercise early at (n,k): " << nodes << "\n";
}

void PrintBlackScholes(std::ostream& os) {
  os << "\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 "
        "==\n";
  os << std::format("C={} P={} delta={}\n",
                    Fmt(BsCall(100.0, 100.0, 0.05, 0.2, 1.0)),
                    Fmt(BsPut(100.0, 100.0, 0.05, 0.2, 1.0)),
                    Fmt(BsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0)));
  for (int num_steps : {10, 50, 100, 500, 1000}) {
    os << std::format(
        "CRR N={}: C={} P={} P_A={}\n", num_steps,
        Fmt(CrrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, num_steps, true)),
        Fmt(CrrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, num_steps, false)),
        Fmt(CrrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, num_steps, false,
                         true)));
  }
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 8: Option Pricing (illustrative numbers, book formulas)\n";
  PrintOneStep(os);
  PrintTwoSteps(os);
  PrintGeneralModel(os);
  PrintAmerican(os);
  PrintBlackScholes(os);
}

}  // namespace m4f::ch08
