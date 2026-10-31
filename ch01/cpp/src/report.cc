#include "m4f/report.h"

#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>

#include "m4f/forward.h"
#include "m4f/fx_arbitrage.h"
#include "m4f/one_step.h"

namespace m4f::ch01 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

void PrintPortfolioLine(std::ostream& os, const OneStepMarket& market,
                        std::string_view name, const Portfolio& portfolio) {
  const double k_up = market.PortfolioReturn(portfolio, State::kUp);
  const double k_down = market.PortfolioReturn(portfolio, State::kDown);
  os << std::format(
      "{}: x={} y={} V(0)={} V(1)={}|{} K_V={}|{}\n", name, Fmt(portfolio.x),
      Fmt(portfolio.y), Fmt(market.Value0(portfolio)),
      Fmt(market.Value1(portfolio, State::kUp)),
      Fmt(market.Value1(portfolio, State::kDown)), Fmt(k_up), Fmt(k_down));
}

void PrintRiskLine(std::ostream& os, const OneStepMarket& market,
                   std::string_view name, const Portfolio& portfolio) {
  const double k_up = market.PortfolioReturn(portfolio, State::kUp);
  const double k_down = market.PortfolioReturn(portfolio, State::kDown);
  os << std::format("{}: x={} y={} V(1)={}|{} K_V={}|{} E(K_V)={} sigma_V={}\n",
                    name, Fmt(portfolio.x), Fmt(portfolio.y),
                    Fmt(market.Value1(portfolio, State::kUp)),
                    Fmt(market.Value1(portfolio, State::kDown)), Fmt(k_up),
                    Fmt(k_down), Fmt(market.Expect(k_up, k_down)),
                    Fmt(market.StdDev(k_up, k_down)));
}

void PrintOptionLine(std::ostream& os, const OneStepMarket& market,
                     std::string_view name, double d_up, double d_down) {
  const Portfolio replica = market.Replicate(d_up, d_down);
  os << std::format("{}: payoff {}|{} replicate x={} y={} price={}\n", name,
                    Fmt(d_up), Fmt(d_down), Fmt(replica.x), Fmt(replica.y),
                    Fmt(market.Price(d_up, d_down)));
}

void PrintArbitrageLines(std::ostream& os, const OneStepMarket& market) {
  os << std::format("d < r < u: {} < {} < {} -> arbitrage-free: {}\n",
                    Fmt(market.d()), Fmt(market.r()), Fmt(market.u()),
                    market.IsArbitrageFree() ? "yes" : "no");
  if (const std::optional<Portfolio> arb = market.Arbitrage()) {
    os << std::format("Arbitrage: x={} y={} V(0)={} V(1)={}|{}\n", Fmt(arb->x),
                      Fmt(arb->y), Fmt(market.Value0(*arb)),
                      Fmt(market.Value1(*arb, State::kUp)),
                      Fmt(market.Value1(*arb, State::kDown)));
  }
}

// Examples 1.1-1.2, Exercises 1.1-1.2 and the no-arbitrage check.
void PrintReturnsAndPortfolios(std::ostream& os) {
  // The book leaves p unspecified (0 < p < 1); it does not affect anything
  // printed here.
  const OneStepMarket ex(100.0, 110.0, 50.0, 52.0, 48.0, 0.5);

  os << "== Example 1.1: returns ==\n";
  os << std::format("A(0)={} A(1)={} S(0)={} S(1)={}|{}\n", Fmt(ex.a0()),
                    Fmt(ex.a1()), Fmt(ex.s0()), Fmt(ex.s1(State::kUp)),
                    Fmt(ex.s1(State::kDown)));
  os << std::format("K_A={} K_S={}|{}\n", Fmt(ex.r()), Fmt(ex.u()),
                    Fmt(ex.d()));

  os << "\n== Example 1.2: portfolio value and return ==\n";
  PrintPortfolioLine(os, ex, "Portfolio", {20.0, 10.0});

  // Exercises 1.1-1.2 (again p is left unspecified).
  const OneStepMarket exr(90.0, 100.0, 25.0, 30.0, 20.0, 0.5);
  os << "\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 ==\n";
  PrintPortfolioLine(os, exr, "Portfolio", {10.0, 15.0});

  os << "\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==\n";
  PrintPortfolioLine(os, exr, "Solution", exr.Replicate(1160.0, 1040.0));

  os << "\n== No-arbitrage check on the Example 1.1 market ==\n";
  PrintArbitrageLines(os, ex);
}

// Exercise 1.3.
void PrintCurrencyArbitrage(std::ostream& os) {
  os << "\n== Exercise 1.3: currency dealers A (New York) and B (London) ==\n";
  const FxMarket fx({{"A", "EUR", "USD", 1.0202, 1.0284},
                     {"A", "GBP", "USD", 1.5718, 1.5844},
                     {"B", "EUR", "GBP", 0.6324, 0.6401},
                     {"B", "USD", "GBP", 0.6299, 0.6375}});
  const FxCycle cycle = fx.BestCycle("USD");
  std::string route = "USD";
  for (const FxLeg& leg : cycle.legs) route += " -> " + leg.to;
  os << "Best round trip: " << route << "\n";
  for (const FxLeg& leg : cycle.legs) {
    os << std::format("  {} -> {} with dealer {}: rate {:.6f}\n", leg.from,
                      leg.to, leg.dealer, leg.rate);
  }
  os << std::format("1 USD -> {:.6f} USD: {}\n", cycle.growth,
                    cycle.growth > 1
                        ? "arbitrage (borrow USD, run the cycle, repay)"
                        : "no arbitrage");
}

// Example 1.4 / Figure 1.1 and the proof of Proposition 1.1.
void PrintBinomialTree(std::ostream& os) {
  // p is left unspecified by the book.
  const OneStepMarket market(100.0, 110.0, 100.0, 125.0, 105.0, 0.5);
  const double s_up = market.s1(State::kUp);
  const double s_down = market.s1(State::kDown);
  os << "\n== Example 1.4: one-step binomial tree (Figure 1.1) ==\n";
  os << std::format("A(0)={} A(1)={} S(0)={} S(1)={}|{}\n", Fmt(market.a0()),
                    Fmt(market.a1()), Fmt(market.s0()), Fmt(s_up), Fmt(s_down));
  os << std::format("K_A={} K_S={}|{}\n", Fmt(market.r()), Fmt(market.u()),
                    Fmt(market.d()));
  os << std::format(
      "Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: {} < {} < {} -> {}\n",
      Fmt(s_down), Fmt(market.a1()), Fmt(s_up),
      s_down < market.a1() && market.a1() < s_up ? "holds" : "fails");
  // Proof of Proposition 1.1: the two violating cases (illustrative S(1)
  // values).
  for (const auto& [up, down, label] :
       {std::tuple{130.0, 110.0, "A(1) <= S^d"},
        std::tuple{110.0, 90.0, "A(1) >= S^u"}}) {
    const OneStepMarket bad(100.0, 110.0, 100.0, up, down, 0.5);
    const Portfolio arb = *bad.Arbitrage();
    os << std::format(
        "  proof, case {} (S(1)={}|{}): x={} y={} V(0)={} V(1)={}|{}\n", label,
        Fmt(up), Fmt(down), Fmt(arb.x), Fmt(arb.y), Fmt(bad.Value0(arb)),
        Fmt(bad.Value1(arb, State::kUp)), Fmt(bad.Value1(arb, State::kDown)));
  }
  PrintArbitrageLines(os, market);
}

// Section 1.4 and Exercise 1.4: risk and return, $10,000 to invest.
void PrintRiskAndReturn(std::ostream& os) {
  const OneStepMarket market(100.0, 110.0, 80.0, 100.0, 60.0, 0.8);
  os << "\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, "
        "budget 10000 ==\n";
  PrintRiskLine(os, market, "Mixed", {50.0, 60.0});
  PrintRiskLine(os, market, "Bonds only", {0.0, 100.0});
  PrintRiskLine(os, market, "Stock only", {125.0, 0.0});
  PrintRiskLine(os, market, "Exercise 1.4 (50/50)", market.Split(10000.0, 0.5));
}

// Section 1.5: Example 1.5, Proposition 1.2, Exercises 1.5-1.6.
void PrintForwards(std::ostream& os) {
  os << "\n== 1.5 Forward contracts ==\n";
  os << std::format("Example 1.5: F={}\n", Fmt(80.0));
  for (double s1 : {84.0, 75.0}) {
    os << std::format("  S(1)={}: long payoff {}, short payoff {}\n", Fmt(s1),
                      Fmt(LongForwardPayoff(s1, 80.0)),
                      Fmt(ShortForwardPayoff(s1, 80.0)));
  }
  os << std::format(
      "Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = {}\n",
      Fmt(ForwardPrice(50.0, 100.0, 110.0)));

  const double s0 = 34.0;
  const double a0 = 100.0;
  const double a1 = 112.0;
  const double quoted = 38.60;
  os << std::format(
      "Exercise 1.5: A=100->112, S(0)=34, quoted F={}, fair F={}\n",
      Fmt(quoted), Fmt(ForwardPrice(s0, a0, a1)));
  const ForwardPortfolio arb = *ForwardArbitrage(s0, a0, a1, quoted);
  os << std::format(
      "  arbitrage (x,y,z)=({},{},{}) V(0)={} V(1)={}|{}|{} for "
      "S(1)=30|40|50\n",
      Fmt(arb.x), Fmt(arb.y), Fmt(arb.z), Fmt(Value0(arb, s0, a0)),
      Fmt(Value1(arb, 30.0, a1, quoted)), Fmt(Value1(arb, 40.0, a1, quoted)),
      Fmt(Value1(arb, 50.0, a1, quoted)));

  const double foreign_rate = ImpliedForeignRate(1.6, 1.50, 0.05);
  os << std::format(
      "Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> sterling "
      "rate {}, bond paying GBP 100 costs GBP {}\n",
      Fmt(foreign_rate), Fmt(100.0 / (1 + foreign_rate)));
}

// Sections 1.6-1.7: options, Exercises 1.7-1.12.
void PrintOptions(std::ostream& os) {
  // p is left unspecified by the book.
  const OneStepMarket market(100.0, 110.0, 100.0, 120.0, 80.0, 0.5);
  const double s_up = market.s1(State::kUp);
  const double s_down = market.s1(State::kDown);
  os << "\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 ==\n";
  PrintOptionLine(os, market, "Call X=100", CallPayoff(s_up, 100.0),
                  CallPayoff(s_down, 100.0));
  for (double strike : {90.0, 110.0}) {
    PrintOptionLine(os, market,
                    std::format("Exercise 1.7 call X={:.0f}", strike),
                    CallPayoff(s_up, strike), CallPayoff(s_down, strike));
  }
  for (double a1 : {105.0, 115.0}) {
    const OneStepMarket market_1_8(100.0, a1, 100.0, 120.0, 80.0, 0.5);
    PrintOptionLine(os, market_1_8,
                    std::format("Exercise 1.8 call X=100, A(1)={:.0f}", a1),
                    CallPayoff(s_up, 100.0), CallPayoff(s_down, 100.0));
  }
  PrintOptionLine(os, market, "Exercise 1.9 put X=100", PutPayoff(s_up, 100.0),
                  PutPayoff(s_down, 100.0));

  const double strike = 100.0;
  const double call_up = CallPayoff(s_up, strike);
  const double call_down = CallPayoff(s_down, strike);
  const double put_up = PutPayoff(s_up, strike);
  const double put_down = PutPayoff(s_down, strike);
  const double call0 = market.Price(call_up, call_down);
  const double put0 = market.Price(put_up, put_down);
  os << std::format("Risk-neutral p*={} -> C(0)={} P(0)={}\n",
                    Fmt(market.RiskNeutralProb()),
                    Fmt(market.RiskNeutralPrice(call_up, call_down)),
                    Fmt(market.RiskNeutralPrice(put_up, put_down)));
  os << std::format("Put-call parity: C(0)-P(0)={} S(0)-X/(1+r)={}\n",
                    Fmt(call0 - put0),
                    Fmt(market.s0() - strike / (1 + market.r())));

  os << "\n== 1.7 Managing risk with options (section 1.6 market, budget "
        "1000) ==\n";
  const double budget = 1000.0;
  const double num_shares = budget / market.s0();
  const double num_calls = budget / call0;
  os << std::format("All in stock: {} shares -> V(1)={}|{} return {}|{}\n",
                    Fmt(num_shares), Fmt(num_shares * s_up),
                    Fmt(num_shares * s_down),
                    Fmt(num_shares * s_up / budget - 1),
                    Fmt(num_shares * s_down / budget - 1));
  os << std::format("All in calls: {} calls  -> V(1)={}|{} return {}|{}\n",
                    Fmt(num_calls), Fmt(num_calls * call_up),
                    Fmt(num_calls * call_down),
                    Fmt(num_calls * call_up / budget - 1),
                    Fmt(num_calls * call_down / budget - 1));
  const double half_shares = budget / 2 / market.s0();
  const double half_calls = budget / 2 / call0;
  const double v_up = half_shares * s_up + half_calls * call_up;
  const double v_down = half_shares * s_down + half_calls * call_down;
  os << std::format(
      "Exercise 1.10 (50/50): {} shares + {} calls -> V(1)={}|{} return "
      "{}|{}\n",
      Fmt(half_shares), Fmt(half_calls), Fmt(v_up), Fmt(v_down),
      Fmt(v_up / budget - 1), Fmt(v_down / budget - 1));
}

// Section 1.7: hedging a future purchase with calls (strike 100),
// Exercises 1.11-1.12.
void PrintHedging(std::ostream& os) {
  const OneStepMarket market(100.0, 110.0, 100.0, 160.0, 40.0, 0.5);
  os << "\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call "
        "X=100\n";
  os << std::format("C(0)={} -> loan repaid at t=1: {}\n",
                    Fmt(market.Price(60.0, 0.0)),
                    Fmt(market.Price(60.0, 0.0) * (1 + market.r())));
  for (double num_calls : {0.0, 1.0, 2.0}) {
    os << std::format(
        "Cost of one share with {:.0f} calls: {}|{}\n", num_calls,
        Fmt(HedgedPurchaseCost(market, 100.0, num_calls, State::kUp)),
        Fmt(HedgedPurchaseCost(market, 100.0, num_calls, State::kDown)));
  }
  for (double p : {0.25, 0.5, 0.75}) {
    const OneStepMarket market_p(100.0, 110.0, 100.0, 160.0, 40.0, p);
    const double sigma_without =
        market_p.StdDev(HedgedPurchaseCost(market_p, 100.0, 0.0, State::kUp),
                        HedgedPurchaseCost(market_p, 100.0, 0.0, State::kDown));
    const double sigma_with =
        market_p.StdDev(HedgedPurchaseCost(market_p, 100.0, 1.0, State::kUp),
                        HedgedPurchaseCost(market_p, 100.0, 1.0, State::kDown));
    os << std::format(
        "Exercise 1.11/1.12 p={}: sigma without option={} with option={} "
        "ratio={}\n",
        Fmt(p), Fmt(sigma_without), Fmt(sigma_with),
        Fmt(sigma_with / sigma_without));
  }
}

}  // namespace

void PrintReport(std::ostream& os) {
  PrintReturnsAndPortfolios(os);
  PrintCurrencyArbitrage(os);
  PrintBinomialTree(os);
  PrintRiskAndReturn(os);
  PrintForwards(os);
  PrintOptions(os);
  PrintHedging(os);
}

}  // namespace m4f::ch01
