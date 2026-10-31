// Unit tests for chapter 9. A tiny self-contained harness keeps the build free
// of third-party dependencies; each check mirrors a Kotlin and Python test.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

#include "m4f/black_scholes.h"
#include "m4f/hedging.h"
#include "m4f/risk.h"

namespace m4f::ch09 {
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

bool Near(double a, double b, double eps = 1e-7) {
  return std::abs(a - b) < eps;
}

constexpr double kS = 100.0;
constexpr double kStrike = 100.0;
constexpr double kRate = 0.05;
constexpr double kSigma = 0.2;
constexpr double kMaturity = 0.5;

void TestNormal(Checker& t) {
  t.Expect(Near(NormCdf(0.0), 0.5), "N(0)");
  t.Expect(Near(NormCdf(1.959963984540054), 0.975, 1e-9), "N(1.96)");
  t.Expect(Near(NormInv(0.975), 1.959963984540054, 1e-9), "N^-1(0.975)");
  t.Expect(Near(NormPdf(0.0), 1 / std::sqrt(2 * std::numbers::pi), 1e-12),
           "pdf(0)");
}

// Greeks against finite differences.
void TestGreeks(Checker& t) {
  const double h = 1e-4;
  for (bool is_call : {true, false}) {
    const Greeks g = BsGreeks(kS, kStrike, kRate, kSigma, kMaturity, is_call);
    auto p = [&](double s, double x, double r, double sigma, double maturity) {
      return BsPrice(s, x, r, sigma, maturity, is_call);
    };
    t.Expect(Near(g.price, p(kS, kStrike, kRate, kSigma, kMaturity)), "price");
    t.Expect(Near(g.delta,
                  (p(kS + h, kStrike, kRate, kSigma, kMaturity) -
                   p(kS - h, kStrike, kRate, kSigma, kMaturity)) /
                      (2 * h),
                  1e-6),
             "delta");
    t.Expect(Near(g.gamma,
                  (p(kS + h, kStrike, kRate, kSigma, kMaturity) -
                   2 * p(kS, kStrike, kRate, kSigma, kMaturity) +
                   p(kS - h, kStrike, kRate, kSigma, kMaturity)) /
                      (h * h),
                  1e-4),
             "gamma");
    t.Expect(Near(g.theta,
                  -(p(kS, kStrike, kRate, kSigma, kMaturity + h) -
                    p(kS, kStrike, kRate, kSigma, kMaturity - h)) /
                      (2 * h),
                  1e-5),
             "theta");
    t.Expect(Near(g.vega,
                  (p(kS, kStrike, kRate, kSigma + h, kMaturity) -
                   p(kS, kStrike, kRate, kSigma - h, kMaturity)) /
                      (2 * h),
                  1e-5),
             "vega");
    t.Expect(Near(g.rho,
                  (p(kS, kStrike, kRate + h, kSigma, kMaturity) -
                   p(kS, kStrike, kRate - h, kSigma, kMaturity)) /
                      (2 * h),
                  1e-5),
             "rho");
  }
}

// Hedges are neutral and self-financing.
void TestHedges(Checker& t) {
  const std::vector<Position> written = {{-1000.0, kStrike, kMaturity, true}};
  const Position extra = {0.0, 110.0, kMaturity, true};
  const Hedge h_delta = DeltaHedge(written, kS, kRate, kSigma);
  const Hedge h_gamma =
      DeltaGreekHedge(written, extra, kS, kRate, kSigma, Greek::kGamma);
  const Hedge h_vega =
      DeltaGreekHedge(written, extra, kS, kRate, kSigma, Greek::kVega);
  for (const Hedge* hedge : {&h_delta, &h_gamma, &h_vega}) {
    t.Expect(Near(PortfolioValue(*hedge, kS, 0.0, kRate, kSigma), 0.0, 1e-8),
             "hedge value 0 at t=0");
    t.Expect(Near(TotalGreek(hedge->options, kS, kRate, kSigma, Greek::kDelta) +
                      hedge->shares,
                  0.0, 1e-9),
             "delta neutral");
  }
  t.Expect(Near(TotalGreek(h_gamma.options, kS, kRate, kSigma, Greek::kGamma),
                0.0, 1e-9),
           "gamma neutral");
  t.Expect(Near(TotalGreek(h_vega.options, kS, kRate, kSigma, Greek::kVega),
                0.0, 1e-6),
           "vega neutral");
  t.Expect(
      std::abs(PortfolioValue(h_delta, kS + 1, 0.0, kRate, kSigma) -
               TotalGreek(written, kS, kRate, kSigma, Greek::kGamma) / 2) < 0.5,
      "delta hedge error ~ gamma/2");

  // Rebalancing is self-financing.
  const auto steps =
      RebalancePath(written, {100.0, 103.0, 101.0}, 1.0 / 52, kRate, kSigma);
  t.Expect(steps.size() == 3 && Near(steps[0].value, 0.0, 1e-8), "path start");
  t.Expect(Near(steps[1].money,
                steps[0].money * std::exp(kRate / 52) -
                    (steps[1].shares - steps[0].shares) * steps[1].s,
                1e-6),
           "path self-financing");
}

void TestLognormal(Checker& t) {
  const Lognormal model = {100.0, 0.08, 0.3, 0.5};
  t.Expect(Near(Cdf(model, Quantile(model, 0.3)), 0.3, 1e-9), "quantile/cdf");
  t.Expect(
      Near(ExpectedCallPayoff(model, 90.0) - ExpectedPutPayoff(model, 90.0),
           ExpectedStock(model) - 90.0, 1e-9),
      "parity");
  const int n = 20000;
  const double lo = -8.0;
  const double hi = 8.0;
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    const double z = lo + (i + 0.5) * (hi - lo) / n;
    const double s = 100.0 * std::exp(0.08 * 0.5 + 0.3 * std::sqrt(0.5) * z);
    total += std::max(s - 90.0, 0.0) * NormPdf(z) * (hi - lo) / n;
  }
  t.Expect(Near(total, ExpectedCallPayoff(model, 90.0), 1e-6),
           "expected call payoff by quadrature");
}

// VaR and strategies.
void TestStrategies(Checker& t) {
  const Lognormal model = {100.0, 0.08, 0.3, 0.5};
  const Strategy stock = UnhedgedHolding(1.0);
  t.Expect(Near(ValueAtRisk(stock, model, kRate, 0.3, 0.95),
                100.0 - Quantile(model, 0.05)),
           "VaR stock");
  const Strategy hedged = PutHedgedHolding("p", 1.0, 95.0);
  t.Expect(Near(ValueAtRisk(hedged, model, kRate, 0.3, 0.95),
                BsPrice(100.0, 95.0, kRate, 0.3, 0.5, false) + 5.0),
           "VaR hedged");
  t.Expect(ExpectedReturn(hedged, model, kRate, 0.3) <
               ExpectedReturn(stock, model, kRate, 0.3),
           "hedging costs return");
  const double v0 = 10000.0;
  for (const Strategy& st :
       {AllInStock(v0, 100.0),
        AllInCalls("c", v0, 100.0, 100.0, kRate, 0.25, 0.5),
        BullSpread("b", v0, 100.0, 100.0, 115.0, kRate, 0.25, 0.5)}) {
    t.Expect(Near(InitialCost(st, 100.0, kRate, 0.25, 0.5), v0),
             "initial cost = V0");
    const double breakeven = Breakeven(st, model, kRate, 0.25);
    t.Expect(Near(TerminalValue(st, breakeven, kRate, 0.5), v0, 1e-6),
             "breakeven");
    t.Expect(Near(ProbLoss(st, model, kRate, 0.25), Cdf(model, breakeven)),
             "P(loss)");
  }
  t.Expect(Near(Breakeven(AllInCalls("c", v0, 100.0, 100.0, kRate, 0.25, 0.5),
                          model, kRate, 0.25),
                100.0 + BsPrice(100.0, 100.0, kRate, 0.25, 0.5, true), 1e-6),
           "call breakeven = X + C");
}

int RunAllTests() {
  Checker t;
  TestNormal(t);
  TestGreeks(t);
  TestHedges(t);
  TestLognormal(t);
  TestStrategies(t);
  if (t.failures() == 0) std::printf("All C++ tests passed\n");
  return t.failures() == 0 ? 0 : 1;
}

}  // namespace
}  // namespace m4f::ch09

int main() { return m4f::ch09::RunAllTests(); }
