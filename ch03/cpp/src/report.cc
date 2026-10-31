#include "m4f/report.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "m4f/binomial_tree.h"
#include "m4f/other_models.h"
#include "m4f/returns.h"

namespace m4f::ch03 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

void PrintReturns(std::ostream& os) {
  os << "\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), "
        "k(n) = ln(S(n) / S(n-1)) ==\n";
  const std::vector<double> path{45.0, 48.0, 47.5, 50.0};
  const std::vector<double> returns = Returns(path);
  const std::vector<double> log_returns = LogReturns(path);
  for (std::size_t n = 1; n < path.size(); ++n) {
    os << std::format("n={}: S={} K={} k={}\n", n, Fmt(path[n]),
                      Fmt(returns[n - 1]), Fmt(log_returns[n - 1]));
  }
  os << std::format("K(0,3) from S: {}, from (1+K(1))(1+K(2))(1+K(3))-1: {}\n",
                    Fmt(OneStepReturn(path[0], path[3])),
                    Fmt(CompoundReturn(returns)));
  os << std::format("k(0,3) from S: {}, k(1)+k(2)+k(3): {}\n",
                    Fmt(LogReturn(path[0], path[3])),
                    Fmt(log_returns[0] + log_returns[1] + log_returns[2]));
  os << std::format("S(0)=45, S(1)=48 with dividend 2: K(1)={}\n",
                    Fmt(OneStepReturn(45.0, 48.0, 2.0)));
}

void PrintExpectedReturn(std::ostream& os) {
  os << "\n== 3.1.2 Expected return ==\n";
  const std::vector<double> prices{52.0, 48.0, 40.0};
  const std::vector<double> probs{0.25, 0.5, 0.25};
  std::vector<double> returns;
  for (double s1 : prices) returns.push_back(OneStepReturn(45.0, s1));
  const double expected_return = Expectation(returns, probs);
  os << std::format(
      "S(0)=45, S(1) in {{52, 48, 40}} with p={{0.25, 0.5, 0.25}}: E(K)={} "
      "sd(K)={}\n",
      Fmt(expected_return), Fmt(std::sqrt(Variance(returns, probs))));
  os << std::format("E(S(1)) = (1 + E(K)) S(0) = {}, directly {}\n",
                    Fmt((1 + expected_return) * 45.0),
                    Fmt(Expectation(prices, probs)));
  os << std::format(
      "Independent steps, E(K(1))=0.10, E(K(2))=0.05: E(K(0,2)) = {}\n",
      Fmt(CompoundReturn({0.10, 0.05})));
}

void PrintBinomialTree(std::ostream& os, const BinomialTree& tree) {
  os << "\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 "
        "==\n";
  for (int n = 0; n < 4; ++n) {
    std::string nodes;
    for (int k = n; k >= 0; --k) {
      if (k != n) nodes += ' ';
      nodes += std::format("{}({})", Fmt(tree.Price(n, k)),
                           Fmt(BinomialProb(n, k, 0.6)));
    }
    os << std::format("n={}: {}\n", n, nodes);
  }
  os << std::format("E(K(n))={} E(S(3))={} (1+E(K))^3 S(0)={}\n",
                    Fmt(tree.ExpectedReturn(0.6)),
                    Fmt(tree.ExpectedPrice(3, 0.6)),
                    Fmt(std::pow(1 + tree.ExpectedReturn(0.6), 3) * 100.0));
}

void PrintRiskNeutral(std::ostream& os, const BinomialTree& tree) {
  os << "\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==\n";
  const double q = tree.RiskNeutralProb();
  os << std::format("d < r < u: {}, p*={}, E*(K)={}\n",
                    tree.IsArbitrageFree() ? "yes" : "no", Fmt(q),
                    Fmt(tree.ExpectedReturn(q)));
  os << std::format("E*(S(1))={} (1+r)S(0)={}; E*(S(3))={} (1+r)^3 S(0)={}\n",
                    Fmt(tree.ExpectedPrice(1, q)), Fmt(1.03 * 100.0),
                    Fmt(tree.ExpectedPrice(3, q)),
                    Fmt(std::pow(1.03, 3) * 100.0));
}

void PrintMartingale(std::ostream& os, const BinomialTree& tree) {
  os << "\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==\n";
  double worst = 0.0;
  for (int n = 0; n < 3; ++n) {
    for (int k = 0; k <= n; ++k) {
      worst = std::max(worst, std::abs(tree.ConditionalDiscounted(n, k) -
                                       tree.DiscountedPrice(n, k)));
    }
  }
  for (int k = 1; k >= 0; --k) {
    os << std::format("node (1,{}): S~(1)={} E*(S~(2)|.)={}\n", k,
                      Fmt(tree.DiscountedPrice(1, k)),
                      Fmt(tree.ConditionalDiscounted(1, k)));
  }
  os << std::format("Largest deviation over all nodes n<3: {}\n", Fmt(worst));
}

void PrintTrinomial(std::ostream& os) {
  os << "\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 ==\n";
  const TrinomialModel model{0.10, 0.0, -0.10, 0.02};
  os << std::format("Risk-neutral probabilities exist for 0 < p_m < {}\n",
                    Fmt(model.MaxMiddleProb()));
  for (double p_mid : {0.2, 0.4, 0.6}) {
    const RiskNeutralProbs probs = model.GetRiskNeutralProbs(p_mid);
    const double call = probs.p_up * 10.0 / 1.02;
    os << std::format(
        "p_m={}: p_u={} p_d={} E*(K)={} call(K=100) price={}\n",
        Fmt(probs.p_mid), Fmt(probs.p_up), Fmt(probs.p_down),
        Fmt(probs.p_up * 0.10 + p_mid * 0.0 + probs.p_down * -0.10), Fmt(call));
  }
}

void PrintContinuousLimit(std::ostream& os) {
  os << "\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 ==\n";
  for (int num_steps : {1, 10, 100, 1000, 10000}) {
    os << std::format(
        "N={}: E(S(T))={} P(S(T)>110)={}\n", num_steps,
        Fmt(BinomialExpectedPrice(100.0, 0.05, 0.2, 1.0, num_steps)),
        Fmt(BinomialProbAbove(100.0, 0.05, 0.2, 1.0, num_steps, 110.0)));
  }
  os << std::format("Limit: E(S(T))={} P(S(T)>110)={}\n",
                    Fmt(LognormalExpectedPrice(100.0, 0.05, 0.2, 1.0)),
                    Fmt(LognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0)));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 3: Risky Assets (illustrative numbers, book formulas)\n";
  PrintReturns(os);
  PrintExpectedReturn(os);
  const BinomialTree tree{100.0, 0.10, -0.05, 0.03};
  PrintBinomialTree(os, tree);
  PrintRiskNeutral(os, tree);
  PrintMartingale(os, tree);
  PrintTrinomial(os);
  PrintContinuousLimit(os);
}

}  // namespace m4f::ch03
