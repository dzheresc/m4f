#include "m4f/report.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "m4f/binomial.h"
#include "m4f/options.h"

namespace m4f::ch07 {
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

// The model price used throughout: r = 0.05, sigma = 0.2, 200 steps.
double Price(double s0, double strike, double maturity, bool is_call,
             bool american = false) {
  return CrrPrice(s0, strike, 0.05, 0.2, maturity, 200, is_call, american);
}

void PrintDefinitions(std::ostream& os) {
  os << "\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==\n";
  for (double s_t : {80.0, 100.0, 120.0}) {
    const double call = CallPayoff(s_t, 100.0);
    const double put = PutPayoff(s_t, 100.0);
    os << std::format(
        "S(T)={}: call payoff={} profit long={} short={}; put payoff={} "
        "profit long={} short={}\n",
        Fmt(s_t), Fmt(call), Fmt(Profit(call, 10.0, 0.05, 1.0)),
        Fmt(-Profit(call, 10.0, 0.05, 1.0)), Fmt(put),
        Fmt(Profit(put, 5.0, 0.05, 1.0)), Fmt(-Profit(put, 5.0, 0.05, 1.0)));
  }
}

void PrintParity(std::ostream& os) {
  os << "\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 "
        "r=0.05 T=1 ==\n";
  os << std::format("S(0) - X e^(-rT) = {}\n",
                    Fmt(100.0 - 100.0 * std::exp(-0.05)));
  const std::vector<std::pair<double, double>> quotes{
      {10.45, 5.57}, {12.0, 5.0}, {9.0, 6.0}};
  for (const auto& [call, put] : quotes) {
    const double gap = ParityGap(call, put, 100.0, 100.0, 0.05, 1.0);
    os << std::format("C={} P={}: gap={} arbitrage profit at T={}\n", Fmt(call),
                      Fmt(put), Fmt(gap), Fmt(std::abs(gap) * std::exp(0.05)));
  }
  os << std::format(
      "Dividend 2 at t=0.5 (PV {}): C - P should be {}\n",
      Fmt(2 * std::exp(-0.025)),
      Fmt(-ParityGap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, 2 * std::exp(-0.025))));
}

void PrintEuropeanBounds(std::ostream& os) {
  os << "\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 "
        "==\n";
  const Bounds call_bounds = EuropeanCallBounds(100.0, 100.0, 0.05, 1.0);
  const Bounds put_bounds = EuropeanPutBounds(100.0, 100.0, 0.05, 1.0);
  os << std::format("call in {}, put in {}\n",
                    FmtList({call_bounds.lo, call_bounds.hi}),
                    FmtList({put_bounds.lo, put_bounds.hi}));
  os << std::format(
      "Quoted call 3.50: violation {}; quoted put 96.00: violation {}; model "
      "call {}: violation {}\n",
      Fmt(BoundViolation(3.5, call_bounds)),
      Fmt(BoundViolation(96.0, put_bounds)),
      Fmt(Price(100.0, 100.0, 1.0, true)),
      Fmt(BoundViolation(Price(100.0, 100.0, 1.0, true), call_bounds)));
}

void PrintAmericanOptions(std::ostream& os) {
  os << "\n== 7.3.2 American calls on a non-dividend paying stock (CRR, "
        "sigma=0.2, N=200) ==\n";
  for (double s0 : {90.0, 100.0, 120.0}) {
    os << std::format("S(0)={}: C_E={} C_A={} exercise now={}\n", Fmt(s0),
                      Fmt(Price(s0, 100.0, 1.0, true)),
                      Fmt(Price(s0, 100.0, 1.0, true, true)),
                      Fmt(CallPayoff(s0, 100.0)));
  }

  os << "\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X "
        "e^(-rT) ==\n";
  for (double s0 : {90.0, 100.0, 120.0}) {
    const double put_e = Price(s0, 100.0, 1.0, false);
    const double put_a = Price(s0, 100.0, 1.0, false, true);
    const double call_a = Price(s0, 100.0, 1.0, true, true);
    const Bounds b = AmericanParityBounds(s0, 100.0, 0.05, 1.0);
    os << std::format("S(0)={}: P_E={} P_A={} C_A-P_A={} in {}\n", Fmt(s0),
                      Fmt(put_e), Fmt(put_a), Fmt(call_a - put_a),
                      FmtList({b.lo, b.hi}));
  }
}

void PrintVariables(std::ostream& os) {
  os << "\n== 7.4.1 European options: dependence on X and S(0) ==\n";
  const std::vector<double> strikes{90.0, 100.0, 110.0};
  std::vector<double> calls;
  std::vector<double> puts;
  for (double strike : strikes) {
    calls.push_back(Price(100.0, strike, 1.0, true));
    puts.push_back(Price(100.0, strike, 1.0, false));
  }
  os << std::format("X={}: C={} P={}\n", FmtList(strikes), FmtList(calls),
                    FmtList(puts));
  os << std::format(
      "C(90)-C(100)={} <= e^(-rT) x 10 = {}; butterfly gap={} (<= 0)\n",
      Fmt(calls[0] - calls[1]), Fmt(10 * std::exp(-0.05)),
      Fmt(ButterflyGap(calls[0], calls[1], calls[2], 90.0, 100.0, 110.0)));
  os << std::format(
      "Quotes C=15, 10, 2: butterfly gap={} -> write 2 x C(100), buy C(90) "
      "and C(110)\n",
      Fmt(ButterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0)));
  std::vector<double> calls_by_spot;
  for (double s : {90.0, 100.0, 110.0}) {
    calls_by_spot.push_back(Price(s, 100.0, 1.0, true));
  }
  os << std::format("S(0)={}: C={}\n", FmtList({90.0, 100.0, 110.0}),
                    FmtList(calls_by_spot));

  os << "\n== 7.4.2 American options: dependence on T ==\n";
  const std::vector<double> maturities{0.25, 0.5, 1.0, 2.0};
  std::vector<double> american_puts;
  for (double maturity : maturities) {
    american_puts.push_back(Price(100.0, 100.0, maturity, false, true));
  }
  os << std::format("T={}: P_A={}\n", FmtList(maturities),
                    FmtList(american_puts));
}

void PrintTimeValue(std::ostream& os) {
  os << "\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 "
        "==\n";
  for (double s0 : {80.0, 100.0, 120.0}) {
    const double call = Price(s0, 100.0, 1.0, true);
    const double put = Price(s0, 100.0, 1.0, false);
    os << std::format("S(0)={}: call {} time value {}; put {} time value {}\n",
                      Fmt(s0), Fmt(call), Fmt(call - CallPayoff(s0, 100.0)),
                      Fmt(put), Fmt(put - PutPayoff(s0, 100.0)));
  }
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 7: Options: General Properties (illustrative numbers, book "
        "formulas)\n";
  PrintDefinitions(os);
  PrintParity(os);
  PrintEuropeanBounds(os);
  PrintAmericanOptions(os);
  PrintVariables(os);
  PrintTimeValue(os);
}

}  // namespace m4f::ch07
