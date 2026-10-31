// Section 6.1, forward contracts, with continuous compounding at rate r.
// Times are in years; F(t, T) is the forward price at t for delivery at T.
#ifndef M4F_CH06_FORWARDS_H_
#define M4F_CH06_FORWARDS_H_

#include <cmath>
#include <utility>
#include <vector>

namespace m4f::ch06 {

// Section 6.1.1: forward price F(t, T) = S(t) e^(r (T - t)), tau = T - t.
inline double ForwardPrice(double s, double r, double tau) {
  return s * std::exp(r * tau);
}

// Dividends (amount, time) paid before T: F(0, T) = (S(0) - PV(div)) e^(r T).
inline double ForwardPriceDividends(
    double s0, double r, double maturity,
    const std::vector<std::pair<double, double>>& dividends) {
  double pv = 0.0;
  for (const auto& [amount, t] : dividends) pv += amount * std::exp(-r * t);
  return (s0 - pv) * std::exp(r * maturity);
}

// Continuous dividend yield r_div (or a foreign interest rate for currencies):
// F(0, T) = S(0) e^((r - r_div) T).
inline double ForwardPriceYield(double s0, double r, double r_div,
                                double maturity) {
  return s0 * std::exp((r - r_div) * maturity);
}

// Riskless profit at T per unit from a mispriced forward. If `quoted` is above
// S(0) e^(rT): borrow S(0), buy the stock, sell forward (cash and carry); if
// below: short the stock, invest S(0), buy forward. Either way the profit is
// |quoted - S(0) e^(rT)|.
inline double ForwardArbitrageProfit(double quoted, double s0, double r,
                                     double maturity) {
  return std::abs(quoted - ForwardPrice(s0, r, maturity));
}

// Section 6.1.2: value at t of a long forward entered at 0,
// V(t) = (F(t,T) - F(0,T)) e^(-r (T-t)).
inline double LongForwardValue(double f0, double ft, double r, double tau) {
  return (ft - f0) * std::exp(-r * tau);
}

}  // namespace m4f::ch06

#endif  // M4F_CH06_FORWARDS_H_
