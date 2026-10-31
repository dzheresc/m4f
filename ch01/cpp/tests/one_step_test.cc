// Unit tests for chapter 1. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include "m4f/one_step.h"

#include <cmath>
#include <cstdio>
#include <optional>

#include "m4f/forward.h"
#include "m4f/fx_arbitrage.h"

namespace m4f::ch01 {
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

bool Near(double a, double b) { return std::abs(a - b) < 1e-9; }

void TestFairMarket(Checker& t) {
  const OneStepMarket m(100, 110, 50, 60, 40, 0.8);
  t.Expect(Near(m.r(), 0.1) && Near(m.u(), 0.2) && Near(m.d(), -0.2),
           "returns");
  t.Expect(m.IsArbitrageFree(), "no arbitrage");
  t.Expect(Near(m.RiskNeutralProb(), 0.75), "p*");
  t.Expect(Near(m.ForwardPrice(), 55), "forward");

  const Portfolio call = m.Replicate(10, 0);
  t.Expect(Near(call.x, 0.5) && Near(call.y, -2.0 / 11), "call replication");
  t.Expect(Near(m.Price(10, 0), 75.0 / 11), "call price 6.8182");
  t.Expect(Near(m.Price(10, 0), m.RiskNeutralPrice(10, 0)),
           "replication == risk-neutral");
  t.Expect(Near(m.Price(10, 0) - m.Price(0, 10), 50 - 50 / 1.1),
           "put-call parity");
}

void TestArbitrage(Checker& t) {
  for (double s0 : {55.0, 36.0}) {
    const OneStepMarket bad(100, 110, s0, 60, 40, 0.8);
    const std::optional<Portfolio> arb = bad.Arbitrage();
    if (!arb) {
      t.Expect(false, "arbitrage portfolio exists");
      continue;
    }
    const double v_up = bad.Value1(*arb, State::kUp);
    const double v_down = bad.Value1(*arb, State::kDown);
    t.Expect(Near(bad.Value0(*arb), 0) && v_up >= 0 && v_down >= 0 &&
                 v_up + v_down > 0,
             "arbitrage portfolio");
  }
  const OneStepMarket fair(100, 110, 50, 60, 40, 0.8);
  t.Expect(!fair.Arbitrage(), "no arbitrage portfolio in fair market");
}

// Book: Examples 1.1-1.2, Exercises 1.1-1.2.
void TestBookExamples(Checker& t) {
  const OneStepMarket ex(100, 110, 50, 52, 48, 0.5);
  const Portfolio p{20, 10};
  t.Expect(Near(ex.u(), 0.04) && Near(ex.d(), -0.04) && !ex.IsArbitrageFree(),
           "Example 1.1");
  t.Expect(Near(ex.Value0(p), 2000) && Near(ex.Value1(p, State::kUp), 2140) &&
               Near(ex.Value1(p, State::kDown), 2060) &&
               Near(ex.PortfolioReturn(p, State::kUp), 0.07) &&
               Near(ex.PortfolioReturn(p, State::kDown), 0.03),
           "Example 1.2");

  const OneStepMarket er(90, 100, 25, 30, 20, 0.5);
  const Portfolio q{10, 15};
  t.Expect(Near(er.Value0(q), 1600) && Near(er.Value1(q, State::kUp), 1800) &&
               Near(er.Value1(q, State::kDown), 1700),
           "Exercise 1.1");
  const Portfolio r = er.Replicate(1160, 1040);
  t.Expect(Near(r.x, 12) && Near(r.y, 8) && Near(er.Value0(r), 1020),
           "Exercise 1.2");
}

// Exercise 1.3: USD -> GBP (A) -> EUR (B) -> USD (A) gains ~0.594%.
void TestCurrencyArbitrage(Checker& t) {
  const FxMarket fx({{"A", "EUR", "USD", 1.0202, 1.0284},
                     {"A", "GBP", "USD", 1.5718, 1.5844},
                     {"B", "EUR", "GBP", 0.6324, 0.6401},
                     {"B", "USD", "GBP", 0.6299, 0.6375}});
  const FxCycle cycle = fx.BestCycle("USD");
  t.Expect(cycle.legs.size() == 3 && cycle.legs[0].to == "GBP" &&
               cycle.legs[1].to == "EUR" &&
               Near(cycle.growth, 1.0202 / (1.5844 * 0.6401)),
           "Exercise 1.3");
}

// Example 1.4 / Proposition 1.1.
void TestExample14(Checker& t) {
  const OneStepMarket m(100, 110, 100, 125, 105, 0.5);
  t.Expect(Near(m.u(), 0.25) && Near(m.d(), 0.05) && m.IsArbitrageFree() &&
               Near(m.RiskNeutralProb(), 0.25) && Near(m.ForwardPrice(), 110),
           "Example 1.4");
}

// Section 1.4 and Exercise 1.4.
void TestRiskAndReturn(Checker& t) {
  const OneStepMarket rr(100, 110, 80, 100, 60, 0.8);
  auto has_moments = [&rr](const Portfolio& p, double mean, double sd) {
    const double k_up = rr.PortfolioReturn(p, State::kUp);
    const double k_down = rr.PortfolioReturn(p, State::kDown);
    return Near(rr.Expect(k_up, k_down), mean) &&
           Near(rr.StdDev(k_up, k_down), sd);
  };
  t.Expect(has_moments({50, 60}, 0.12, 0.08) &&
               has_moments({0, 100}, 0.10, 0.0) &&
               has_moments({125, 0}, 0.15, 0.20),
           "Section 1.4");
  const Portfolio half = rr.Split(10000, 0.5);
  t.Expect(
      Near(half.x, 62.5) && Near(half.y, 50) && has_moments(half, 0.125, 0.10),
      "Exercise 1.4");
}

// Section 1.5: Exercises 1.5 and 1.6.
void TestForwards(Checker& t) {
  const std::optional<ForwardPortfolio> arb =
      ForwardArbitrage(34, 100, 112, 38.60);
  t.Expect(arb && Near(Value0(*arb, 34, 100), 0) &&
               Near(Value1(*arb, 30, 112, 38.60), 0.52) &&
               Near(Value1(*arb, 50, 112, 38.60), 0.52),
           "Exercise 1.5");
  t.Expect(!ForwardArbitrage(34, 100, 112, 38.08),
           "fair forward has no arbitrage");
  const double foreign_rate = ImpliedForeignRate(1.6, 1.5, 0.05);
  t.Expect(
      Near(foreign_rate, 0.12) && Near(100 / (1 + foreign_rate), 100 / 1.12),
      "Exercise 1.6");
}

// Section 1.6: Exercises 1.7-1.9.
void TestOptions(Checker& t) {
  const OneStepMarket o(100, 110, 100, 120, 80, 0.5);
  t.Expect(Near(o.Price(20, 0), 150.0 / 11) &&
               Near(o.Price(30, 0), 225.0 / 11) &&
               Near(o.Price(10, 0), 75.0 / 11),
           "call X=100, Exercise 1.7");
  t.Expect(Near(OneStepMarket(100, 105, 100, 120, 80, 0.5).Price(20, 0),
                50 - 4000.0 / 105) &&
               Near(OneStepMarket(100, 115, 100, 120, 80, 0.5).Price(20, 0),
                    50 - 4000.0 / 115),
           "Exercise 1.8");
  t.Expect(Near(o.Price(0, 20), 50.0 / 11), "Exercise 1.9");
}

// Section 1.7: hedging a future purchase, Exercises 1.11-1.12.
void TestHedging(Checker& t) {
  const OneStepMarket h(100, 110, 100, 160, 40, 0.5);
  t.Expect(Near(h.Price(60, 0), 350.0 / 11), "C(0)=31.8182");
  t.Expect(Near(HedgedPurchaseCost(h, 100, 1, State::kUp), 135) &&
               Near(HedgedPurchaseCost(h, 100, 1, State::kDown), 75) &&
               Near(HedgedPurchaseCost(h, 100, 2, State::kUp), 110) &&
               Near(HedgedPurchaseCost(h, 100, 2, State::kDown), 110),
           "hedged cost");
  for (double p : {0.1, 0.25, 0.5, 0.75, 0.9}) {
    const OneStepMarket hp(100, 110, 100, 160, 40, p);
    const double sigma_without = hp.StdDev(160, 40);
    const double sigma_with =
        hp.StdDev(HedgedPurchaseCost(hp, 100, 1, State::kUp),
                  HedgedPurchaseCost(hp, 100, 1, State::kDown));
    t.Expect(Near(sigma_with, sigma_without / 2), "Exercise 1.12");
  }
}

int RunAllTests() {
  Checker t;
  TestFairMarket(t);
  TestArbitrage(t);
  TestBookExamples(t);
  TestCurrencyArbitrage(t);
  TestExample14(t);
  TestRiskAndReturn(t);
  TestForwards(t);
  TestOptions(t);
  TestHedging(t);
  if (t.failures() == 0) {
    std::printf("all tests passed\n");
    return 0;
  }
  std::printf("%d failure(s)\n", t.failures());
  return 1;
}

}  // namespace
}  // namespace m4f::ch01

int main() { return m4f::ch01::RunAllTests(); }
