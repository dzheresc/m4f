#include "m4f/report.h"

#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "m4f/black_scholes.h"
#include "m4f/hedging.h"
#include "m4f/risk.h"

namespace m4f::ch09 {
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

std::vector<double> LegQtys(const Strategy& st) {
  std::vector<double> q;
  for (const OptionLeg& l : st.legs) q.push_back(l.qty);
  return q;
}

const char* GreekName(Greek g) {
  switch (g) {
    case Greek::kDelta:
      return "delta";
    case Greek::kGamma:
      return "gamma";
    case Greek::kTheta:
      return "theta";
    case Greek::kVega:
      return "vega";
    case Greek::kRho:
      return "rho";
  }
  return "";
}

// The option position of Section 9.1: S(0)=100 X=100 r=0.05 sigma=0.2 T=0.5.
constexpr double kS0 = 100.0;
constexpr double kStrike = 100.0;
constexpr double kRate = 0.05;
constexpr double kSigma = 0.2;
constexpr double kMaturity = 0.5;

const std::vector<double>& Moves() {
  static const auto* const kMoves =
      new std::vector<double>{90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0};
  return *kMoves;
}

const std::vector<Position>& Written() {
  static const auto* const kWritten =
      new std::vector<Position>{{-1000.0, kStrike, kMaturity, true}};
  return *kWritten;
}

void PrintDeltaHedging(std::ostream& os) {
  os << "\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 "
        "r=0.05 sigma=0.2 ==\n";
  const double call = BsPrice(kS0, kStrike, kRate, kSigma, kMaturity, true);
  const Hedge h = DeltaHedge(Written(), kS0, kRate, kSigma);
  os << std::format(
      "Call price C={}, delta={}\n", Fmt(call),
      Fmt(BsGreeks(kS0, kStrike, kRate, kSigma, kMaturity, true).delta));
  os << std::format(
      "Premium received {}; buy {} shares, money market {} (negative = "
      "borrow)\n",
      Fmt(1000.0 * call), Fmt(h.shares), Fmt(h.money));
  os << "Value of the position right after a jump in S (t=0): naked writer vs "
        "delta hedged\n";
  for (double s : Moves()) {
    const double naked =
        1000.0 * call -
        1000.0 * BsPrice(s, kStrike, kRate, kSigma, kMaturity, true);
    os << std::format("S={}: naked {} hedged {}\n", Fmt(s), Fmt(naked),
                      Fmt(PortfolioValue(h, s, 0.0, kRate, kSigma)));
  }
  os << "Same jumps one week later (t=1/52), no rebalancing:\n";
  std::string row = " ";
  for (double s : Moves()) {
    row += std::format(" S={}:{}", Fmt(s),
                       Fmt(PortfolioValue(h, s, 1.0 / 52, kRate, kSigma)));
  }
  os << row << "\n";

  os << "\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced "
        "each week ==\n";
  const std::vector<double> path = {100.0, 103.0, 101.0, 98.0, 102.0, 104.0};
  for (const RebalanceStep& st :
       RebalancePath(Written(), path, 1.0 / 52, kRate, kSigma)) {
    os << std::format("t={} S={}: shares={} money={} value={}\n", Fmt(st.t),
                      Fmt(st.s), Fmt(st.shares), Fmt(st.money), Fmt(st.value));
  }
}

void PrintGreeks(std::ostream& os) {
  os << "\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 "
        "==\n";
  for (bool is_call : {true, false}) {
    const Greeks g = BsGreeks(kS0, kStrike, kRate, kSigma, kMaturity, is_call);
    os << std::format(
        "{}: price={} delta={} gamma={} theta={} vega={} rho={}\n",
        is_call ? "call" : "put", Fmt(g.price), Fmt(g.delta), Fmt(g.gamma),
        Fmt(g.theta), Fmt(g.vega), Fmt(g.rho));
  }
  const std::vector<Greek> all = {Greek::kDelta, Greek::kGamma, Greek::kTheta,
                                  Greek::kVega, Greek::kRho};
  std::string row = "Position Greeks of the written calls:";
  for (Greek g : all) {
    row += std::format(" {}={}", GreekName(g),
                       Fmt(TotalGreek(Written(), kS0, kRate, kSigma, g)));
  }
  os << row << "\n";
}

void PrintGreekHedging(std::ostream& os) {
  os << "\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 "
        "T=0.5 ==\n";
  const Position extra = {0.0, 110.0, kMaturity, true};
  const Hedge h = DeltaHedge(Written(), kS0, kRate, kSigma);
  const Hedge h_gamma =
      DeltaGreekHedge(Written(), extra, kS0, kRate, kSigma, Greek::kGamma);
  const Hedge h_vega =
      DeltaGreekHedge(Written(), extra, kS0, kRate, kSigma, Greek::kVega);
  const std::vector<std::pair<const char*, const Hedge*>> hedges = {
      {"delta", &h}, {"delta-gamma", &h_gamma}, {"delta-vega", &h_vega}};
  for (const auto& [label, hedge] : hedges) {
    const double second =
        hedge->options.size() > 1 ? hedge->options[1].qty : 0.0;
    os << std::format(
        "{}: shares={} calls X=110: {} money={} gamma={} vega={}\n", label,
        Fmt(hedge->shares), Fmt(second), Fmt(hedge->money),
        Fmt(TotalGreek(hedge->options, kS0, kRate, kSigma, Greek::kGamma)),
        Fmt(TotalGreek(hedge->options, kS0, kRate, kSigma, Greek::kVega)));
  }
  os << "Value after an immediate jump in S:\n";
  for (double s : Moves()) {
    os << std::format("S={}: delta {} delta-gamma {} delta-vega {}\n", Fmt(s),
                      Fmt(PortfolioValue(h, s, 0.0, kRate, kSigma)),
                      Fmt(PortfolioValue(h_gamma, s, 0.0, kRate, kSigma)),
                      Fmt(PortfolioValue(h_vega, s, 0.0, kRate, kSigma)));
  }
  os << "Value after an immediate change in sigma (S unchanged):\n";
  for (double sigma : {0.15, 0.2, 0.25, 0.3}) {
    os << std::format("sigma={}: delta {} delta-gamma {} delta-vega {}\n",
                      Fmt(sigma),
                      Fmt(PortfolioValue(h, kS0, 0.0, kRate, sigma)),
                      Fmt(PortfolioValue(h_gamma, kS0, 0.0, kRate, sigma)),
                      Fmt(PortfolioValue(h_vega, kS0, 0.0, kRate, sigma)));
  }

  os << "\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==\n";
  for (const auto& [label, hedge] : hedges) {
    os << std::format("{}: {}\n", label,
                      Fmt(PortfolioValue(*hedge, 97.0, 1.0 / 52, kRate, 0.22)));
  }
  os << "Theta: value change over one week with S and sigma unchanged\n";
  for (const auto& [label, hedge] : hedges) {
    os << std::format(
        "{}: {}  (theta x dt = {})\n", label,
        Fmt(PortfolioValue(*hedge, kS0, 1.0 / 52, kRate, kSigma)),
        Fmt(TotalGreek(hedge->options, kS0, kRate, kSigma, Greek::kTheta) /
            52));
  }
}

void PrintValueAtRisk(std::ostream& os, const Lognormal& model) {
  os << "\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 "
        "(lognormal) ==\n";
  const Strategy stock = UnhedgedHolding(1.0);
  os << std::format("E S(T)={}\n", Fmt(ExpectedStock(model)));
  for (double conf : {0.9, 0.95, 0.99}) {
    os << std::format("confidence {}: z={} S quantile={} VaR={}\n", Fmt(conf),
                      Fmt(NormInv(1 - conf)), Fmt(Quantile(model, 1 - conf)),
                      Fmt(ValueAtRisk(stock, model, kRate, kSigma, conf)));
  }
  os << "Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:\n";
  for (double strike : {85.0, 90.0, 95.0, 100.0}) {
    const Strategy st = PutHedgedHolding(
        std::format("put X={}", static_cast<int>(strike)), 1.0, strike);
    os << std::format(
        "X={}: put price={} initial value={} VaR={} E V(T)={} expected "
        "return={}\n",
        Fmt(strike), Fmt(BsPrice(100.0, strike, kRate, 0.3, 0.5, false)),
        Fmt(InitialCost(st, 100.0, kRate, 0.3, 0.5)),
        Fmt(ValueAtRisk(st, model, kRate, 0.3, 0.95)),
        Fmt(ExpectedValue(st, model, kRate)),
        Fmt(ExpectedReturn(st, model, kRate, 0.3)));
  }
  const Strategy half = PutHedgedHolding("half hedged X=95", 1.0, 95.0, 0.5);
  os << std::format("Half hedged X=95: VaR={} expected return={}\n",
                    Fmt(ValueAtRisk(half, model, kRate, 0.3, 0.95)),
                    Fmt(ExpectedReturn(half, model, kRate, 0.3)));

  os << "\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) "
        "target 100000 ==\n";
  const Strategy big = UnhedgedHolding(10000.0);
  os << std::format("Unhedged: VaR={} expected return={}\n",
                    Fmt(ValueAtRisk(big, model, kRate, 0.3, 0.95)),
                    Fmt(ExpectedReturn(big, model, kRate, 0.3)));
  std::string chosen;
  for (double strike : {80.0, 85.0, 90.0, 95.0, 100.0, 105.0}) {
    const Strategy st = PutHedgedHolding(
        std::format("put X={}", static_cast<int>(strike)), 10000.0, strike);
    const double var = ValueAtRisk(st, model, kRate, 0.3, 0.95);
    const double cost =
        10000.0 * BsPrice(100.0, strike, kRate, 0.3, 0.5, false);
    os << std::format("puts X={}: cost={} VaR={} expected return={}\n",
                      Fmt(strike), Fmt(cost), Fmt(var),
                      Fmt(ExpectedReturn(st, model, kRate, 0.3)));
    if (chosen.empty() && var <= 100000.0) chosen = st.name;
  }
  os << std::format("Cheapest hedge meeting the target: {}\n", chosen);
}

void PrintSpeculating(std::ostream& os) {
  os << "\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 "
        "T=0.5, r=0.05 ==\n";
  const Lognormal spec = {100.0, 0.10, 0.25, 0.5};
  const Strategy stock = AllInStock(10000.0, 100.0);
  const Strategy calls100 =
      AllInCalls("calls X=100", 10000.0, 100.0, 100.0, kRate, 0.25, 0.5);
  const Strategy calls110 =
      AllInCalls("calls X=110", 10000.0, 100.0, 110.0, kRate, 0.25, 0.5);
  const Strategy half_mix =
      Mix("half stock, half calls X=100", stock, 0.5, calls100, 0.5);
  const std::vector<Strategy> strategies = {stock, calls100, calls110,
                                            half_mix};
  os << std::format("C(X=100)={} C(X=110)={}\n",
                    Fmt(BsPrice(100.0, 100.0, kRate, 0.25, 0.5, true)),
                    Fmt(BsPrice(100.0, 110.0, kRate, 0.25, 0.5, true)));
  for (const Strategy& st : strategies) {
    os << std::format("{}: shares={} options={}\n", st.name, Fmt(st.shares),
                      FmtList(LegQtys(st)));
  }
  const std::vector<double> scenarios = {80.0,  90.0,  100.0,
                                         110.0, 120.0, 130.0};
  os << "Terminal value by scenario S(T)=" << FmtList(scenarios) << ":\n";
  for (const Strategy& st : strategies) {
    std::vector<double> values;
    for (double s : scenarios)
      values.push_back(TerminalValue(st, s, kRate, 0.5));
    os << std::format("{}: {}\n", st.name, FmtList(values));
  }
  os << "Expected return, probability of loss, breakeven S(T), VaR at 95%:\n";
  for (const Strategy& st : strategies) {
    os << std::format("{}: E ret={} P(loss)={} breakeven={} VaR={}\n", st.name,
                      Fmt(ExpectedReturn(st, spec, kRate, 0.25)),
                      Fmt(ProbLoss(st, spec, kRate, 0.25)),
                      Fmt(Breakeven(st, spec, kRate, 0.25)),
                      Fmt(ValueAtRisk(st, spec, kRate, 0.25, 0.95)));
  }
}

void PrintBullCase(std::ostream& os) {
  os << "\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months "
        "(m=0.4 sigma=0.25 T=0.25) ==\n";
  const Lognormal bull = {100.0, 0.4, 0.25, 0.25};
  const std::vector<Strategy> strategies = {
      AllInStock(10000.0, 100.0),
      AllInCalls("calls X=100", 10000.0, 100.0, 100.0, kRate, 0.25, 0.25),
      BullSpread("bull spread 100/115", 10000.0, 100.0, 100.0, 115.0, kRate,
                 0.25, 0.25)};
  os << std::format("E S(T)={} P(S(T)>115)={}\n", Fmt(ExpectedStock(bull)),
                    Fmt(1 - Cdf(bull, 115.0)));
  for (const Strategy& st : strategies) {
    os << std::format(
        "{}: options={} E ret={} P(loss)={} VaR={} value at S(T)=115: {}\n",
        st.name, FmtList(LegQtys(st)),
        Fmt(ExpectedReturn(st, bull, kRate, 0.25)),
        Fmt(ProbLoss(st, bull, kRate, 0.25)),
        Fmt(ValueAtRisk(st, bull, kRate, 0.25, 0.95)),
        Fmt(TerminalValue(st, 115.0, kRate, 0.25)));
  }
  std::string row = "If instead the stock stays flat (S(T)=100):";
  for (const Strategy& st : strategies) {
    row += std::format(" {}={}", st.name,
                       Fmt(TerminalValue(st, 100.0, kRate, 0.25)));
  }
  os << row << "\n";
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 9: Financial Engineering (illustrative numbers, book "
        "formulas)\n";
  PrintDeltaHedging(os);
  PrintGreeks(os);
  PrintGreekHedging(os);
  PrintValueAtRisk(os, {100.0, 0.08, 0.3, 0.5});
  PrintSpeculating(os);
  PrintBullCase(os);
}

}  // namespace m4f::ch09
