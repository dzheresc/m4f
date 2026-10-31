#include "m4f/report.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>

#include "m4f/forwards.h"
#include "m4f/futures.h"

namespace m4f::ch06 {
namespace {

// Four decimals, round-half-even on the exact double. Adding 0.0 turns -0.0
// into 0.0 so that no "-0.0000" appears.
std::string Fmt(double v) { return std::format("{:.4f}", v + 0.0); }

// "[a, b, c]" with each entry formatted by Fmt().
std::string FmtList(const Vec& values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out += (i ? ", " : "") + Fmt(values[i]);
  }
  return out + "]";
}

void PrintForwardPrice(std::ostream& os, double f0) {
  os << "\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 "
        "T=0.75 ==\n";
  os << std::format("No dividends: F(0,T)={}\n", Fmt(f0));
  os << std::format(
      "Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)={}\n",
      Fmt(ForwardPriceDividends(100.0, 0.08, 0.75, {{2.0, 0.25}, {2.0, 0.5}})));
  os << std::format("Dividend yield 3%: F(0,T)={}\n",
                    Fmt(ForwardPriceYield(100.0, 0.08, 0.03, 0.75)));
  os << std::format(
      "Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: F={}\n",
      Fmt(ForwardPriceYield(1.25, 0.05, 0.03, 1.0)));
  for (double quoted : {108.0, 104.0}) {
    const char* side = quoted > f0 ? "cash and carry: buy stock, sell forward"
                                   : "reverse: short stock, buy forward";
    os << std::format("Quoted F={}: {}, profit at T={}\n", Fmt(quoted), side,
                      Fmt(ForwardArbitrageProfit(quoted, 100.0, 0.08, 0.75)));
  }
}

void PrintForwardValue(std::ostream& os, double f0) {
  os << "\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) "
        "e^(-r(T-t)) ==\n";
  for (double st : {104.0, 98.0}) {
    const double ft = ForwardPrice(st, 0.08, 0.25);
    os << std::format("t=0.5 S(t)={}: F(t,T)={} long V(t)={} short V(t)={}\n",
                      Fmt(st), Fmt(ft),
                      Fmt(LongForwardValue(f0, ft, 0.08, 0.25)),
                      Fmt(-LongForwardValue(f0, ft, 0.08, 0.25)));
  }
}

void PrintFuturesPricing(std::ostream& os) {
  os << "\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 "
        "d=-0.05 r=0.03 N=3 ==\n";
  const auto tree = FuturesTree(100.0, 0.10, -0.05, 0.03, 3);
  for (int n = 0; n < 4; ++n) {
    Vec futures;
    Vec forward;
    for (int k = n; k >= 0; --k) {
      futures.push_back(tree[n][k]);
      forward.push_back(100.0 * std::pow(1.1, k) * std::pow(0.95, n - k) *
                        std::pow(1.03, 3 - n));
    }
    os << std::format("n={}: futures={} forward={}\n", n, FmtList(futures),
                      FmtList(forward));
  }
  os << "Marking to market: 100 units, initial margin 1000, maintenance 750\n";
  const Vec prices{106.0, 104.5, 101.0, 103.5, 99.0, 102.0};
  const auto days = MarginAccount(prices, 100.0, 1000.0, 750.0);
  for (std::size_t n = 1; n <= days.size(); ++n) {
    os << std::format("day {}: f={} balance={} margin call={}\n", n,
                      Fmt(prices[n]), Fmt(days[n - 1].balance),
                      Fmt(days[n - 1].call));
  }
}

void PrintHedging(std::ostream& os) {
  os << "\n== 6.2.2 Hedging with futures: four equally likely scenarios ==\n";
  const Vec ds{-6.0, -1.0, 2.0, 5.0};
  const Vec df{-5.0, -2.0, 3.0, 4.0};
  const Vec p{0.25, 0.25, 0.25, 0.25};
  const double h = MinVarianceHedgeRatio(ds, df, p);
  os << std::format("dS={} df={}\n", FmtList(ds), FmtList(df));
  os << std::format("h*={}; std of spot change: unhedged {}, h=1 {}, h* {}\n",
                    Fmt(h), Fmt(HedgedStd(0.0, ds, df, p)),
                    Fmt(HedgedStd(1.0, ds, df, p)),
                    Fmt(HedgedStd(h, ds, df, p)));
  os << std::format(
      "Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 "
      "or 95 -> cost {}\n",
      FmtList({112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)}));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 6: Forward and Futures Contracts (illustrative numbers, book "
        "formulas)\n";
  const double f0 = ForwardPrice(100.0, 0.08, 0.75);
  PrintForwardPrice(os, f0);
  PrintForwardValue(os, f0);
  PrintFuturesPricing(os);
  PrintHedging(os);
}

}  // namespace m4f::ch06
