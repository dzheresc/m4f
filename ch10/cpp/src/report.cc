#include "m4f/report.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "m4f/bonds.h"
#include "m4f/term_structure.h"

namespace m4f::ch10 {
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

constexpr double kY0 = 0.06;
constexpr double kV0 = 100000.0;

void PrintSingleBonds(std::ostream& os) {
  os << "\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 "
        "years, y=0.06 continuous ==\n";
  const Bond bond = CouponBond(100.0, 5.0, 4);
  os << std::format("Price at y=0.06: {}\n", Fmt(Price(bond, kY0)));
  os << "Yield jumps right after purchase; value of the investment at horizon "
        "t (coupons reinvested):\n";
  for (double y : {0.05, 0.06, 0.07}) {
    std::string row = std::format("y={}:", Fmt(y));
    for (int t = 1; t <= 4; ++t) {
      row += std::format(" t={}:{}", t, Fmt(InvestmentValue(bond, y, t)));
    }
    os << row << "\n";
  }
  os << "Annualised log return over [0,t]:\n";
  for (double y : {0.05, 0.06, 0.07}) {
    std::string row = std::format("y={}:", Fmt(y));
    for (int t = 1; t <= 4; ++t) {
      row += std::format(" t={}:{}", t, Fmt(LogReturn(bond, kY0, y, t)));
    }
    os << row << "\n";
  }
}

void PrintDuration(std::ostream& os) {
  os << "\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P "
        "==\n";
  const Bond bond = CouponBond(100.0, 5.0, 4);
  const double d = Duration(bond, kY0);
  os << std::format("Coupon bond: D={} years; zero-coupon 4y: D={}\n", Fmt(d),
                    Fmt(Duration(ZeroCoupon(100.0, 4.0), kY0)));
  const double h = 1e-4;
  os << std::format(
      "dP/dy by finite difference: {}, -D P = {}\n",
      Fmt((Price(bond, kY0 + h) - Price(bond, kY0 - h)) / (2 * h)),
      Fmt(-d * Price(bond, kY0)));
  os << "Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first "
        "order): "
     << FmtList({InvestmentValue(bond, 0.05, d), InvestmentValue(bond, 0.06, d),
                 InvestmentValue(bond, 0.07, d)})
     << "\n";
  const std::vector<std::pair<const char*, Bond>> named = {
      {"5y 4%", CouponBond(100.0, 4.0, 5)},
      {"10y 8%", CouponBond(100.0, 8.0, 10)},
      {"10y zero", ZeroCoupon(100.0, 10.0)}};
  for (const auto& [name, b] : named) {
    os << std::format("{}: price={} D={} at y=0.06; D at y=0.08: {}\n", name,
                      Fmt(Price(b, kY0)), Fmt(Duration(b, kY0)),
                      Fmt(Duration(b, 0.08)));
  }
}

void PrintPortfolios(std::ostream& os, const Bond& a, const Bond& b) {
  os << "\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 "
        "years, y=0.06 ==\n";
  const double d_a = Duration(a, kY0);
  const double d_b = Duration(b, kY0);
  const auto [w_a, w_b] = ImmunisingWeights(d_a, d_b, 5.0);
  os << std::format("D_A={} D_B={}: weights w_A={} w_B={} give D={}\n",
                    Fmt(d_a), Fmt(d_b), Fmt(w_a), Fmt(w_b),
                    Fmt(w_a * d_a + w_b * d_b));
  const Portfolio p = Immunise(kV0, a, b, kY0, 0.0, 5.0);
  os << std::format("Invest {}: {} of A and {} of B\n", Fmt(kV0), Fmt(p.q1),
                    Fmt(p.q2));
  const double target = kV0 * std::exp(kY0 * 5.0);
  os << std::format("Target at t=5 if y stays 0.06: {}\n", Fmt(target));
  os << "Value at t=5 after an immediate jump of y (coupons reinvested at the "
        "new yield):\n";
  for (double y : {0.04, 0.05, 0.06, 0.07, 0.08}) {
    const double v =
        p.q1 * InvestmentValue(a, y, 5.0) + p.q2 * InvestmentValue(b, y, 5.0);
    os << std::format(
        "y={}: portfolio {} (shortfall {}), B alone {}, A alone {}\n", Fmt(y),
        Fmt(v), Fmt(target - v),
        Fmt(kV0 / Price(b, kY0) * InvestmentValue(b, y, 5.0)),
        Fmt(kV0 / Price(a, kY0) * InvestmentValue(a, y, 5.0)));
  }
}

void PrintDynamicHedging(std::ostream& os, const Bond& a, const Bond& b) {
  os << "\n== 10.1.4 Dynamic hedging: re-immunise every half year along a "
        "yield path ==\n";
  const std::vector<std::pair<double, double>> path = {
      {0.0, 0.06},  {0.5, 0.065}, {1.0, 0.07},  {1.5, 0.06},
      {2.0, 0.05},  {2.5, 0.055}, {3.0, 0.065}, {3.5, 0.07},
      {4.0, 0.075}, {4.5, 0.07},  {5.0, 0.065}};
  for (const HedgeStep& st : DynamicHedge(kV0, a, b, 5.0, path)) {
    os << std::format("t={} y={}: value={} q_A={} q_B={} D={}\n", Fmt(st.t),
                      Fmt(st.y), Fmt(st.value), Fmt(st.q1), Fmt(st.q2),
                      Fmt(st.duration));
  }
  const auto fixed = DynamicHedge(kV0, a, b, 5.0, path, false);
  os << std::format("At t=5: rebalanced {}, not rebalanced {}, target {}\n",
                    Fmt(DynamicHedge(kV0, a, b, 5.0, path).back().value),
                    Fmt(fixed.back().value), Fmt(kV0 * std::exp(kY0 * 5.0)));
}

void PrintTermStructure(std::ostream& os, const std::vector<double>& maturities,
                        const std::vector<double>& prices) {
  os << "\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==\n";
  const std::vector<double> forwards = ForwardRates(maturities, prices);
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    os << std::format("T={}: B={} y(0,T)={} f(0,T-0.5,T)={}\n",
                      Fmt(maturities[i]), Fmt(prices[i]),
                      Fmt(YieldRate(prices[i], maturities[i])),
                      Fmt(forwards[i]));
  }
  os << std::format("Prices rebuilt from the forward rates: {}\n",
                    FmtList(PricesFromForwardRates(maturities, forwards)));
  double avg = 0.0;
  for (std::size_t i = 2; i < 6; ++i) avg += forwards[i];
  os << std::format("f(0,1,3)={} = average of the half-year forwards {}\n",
                    Fmt(ForwardRate(prices[1], 1.0, prices[5], 3.0)),
                    Fmt(avg / 4));
  std::string row = "Implied B(1,T) = B(0,T)/B(0,1):";
  for (std::size_t i = 2; i < maturities.size(); ++i) {
    row += std::format(" T={}:{}", Fmt(maturities[i]),
                       Fmt(ImpliedPrice(prices[1], prices[i])));
  }
  os << row << "\n";
  os << std::format(
      "FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)={}: buy 1000 "
      "one-year bonds (cost {}), sell {} two-year bonds; repay {}\n",
      Fmt(ForwardRate(prices[1], 1.0, prices[3], 2.0)), Fmt(1000.0 * prices[1]),
      Fmt(1000.0 * prices[1] / prices[3]),
      Fmt(1000.0 * std::exp(ForwardRate(prices[1], 1.0, prices[3], 2.0))));
  const std::vector<double> flow_times = {1.0, 2.0, 3.0};
  const std::vector<double> flow_amounts = {6.0, 6.0, 106.0};
  const double p =
      CouponBondPrice(flow_times, flow_amounts, maturities, prices);
  os << std::format(
      "3-year 6% coupon bond under the term structure: price {}, yield to "
      "maturity {} (between y(0,1)={} and y(0,3)={})\n",
      Fmt(p), Fmt(YieldToMaturity(p, flow_times, flow_amounts)),
      Fmt(YieldRate(prices[1], 1.0)), Fmt(YieldRate(prices[5], 3.0)));

  os << "\n== 10.2.2 Money market account: rolling over half-year bonds at the "
        "forward rates ==\n";
  const std::vector<double> account = MoneyMarketAccount(maturities, prices);
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    os << std::format("A({})={} = 1/B(0,T)={}\n", Fmt(maturities[i]),
                      Fmt(account[i]), Fmt(1 / prices[i]));
  }
  double sum_f = 0.0;
  for (double f : forwards) sum_f += f * 0.5;
  os << std::format(
      "Short rates realised = forward rates: {}; A(3)=exp(sum f dt)={}\n",
      FmtList(forwards), Fmt(std::exp(sum_f)));
  os << std::format(
      "1 unit at t=0 in the 3-year bond: {} at t=3, same as the money market "
      "account {}\n",
      Fmt(1 / prices[5]), Fmt(account[5]));
}

}  // namespace

void PrintReport(std::ostream& os) {
  os << "Chapter 10: Variable Interest Rates (illustrative numbers, book "
        "formulas)\n";
  PrintSingleBonds(os);
  PrintDuration(os);
  const Bond a = CouponBond(100.0, 4.0, 5);
  const Bond b = CouponBond(100.0, 8.0, 10);
  PrintPortfolios(os, a, b);
  PrintDynamicHedging(os, a, b);
  PrintTermStructure(os, {0.5, 1.0, 1.5, 2.0, 2.5, 3.0},
                     {0.978, 0.953, 0.926, 0.898, 0.870, 0.842});
}

}  // namespace m4f::ch10
