// Section 2.2, money market: zero-coupon bonds, coupon bonds and the money
// market account. Bond prices are per unit face value unless a face value is
// given.
//
// Book notation: T is the maturity (`maturity`), B a unit bond price
// (`bond_price`), F the face value (`face`), C the coupon (`coupon`), y the
// yield per period, A(0) the initial balance (`a0`).
#ifndef M4F_CH02_MONEY_MARKET_H_
#define M4F_CH02_MONEY_MARKET_H_

#include <cmath>
#include <optional>
#include <vector>

namespace m4f::ch02 {

// 2.2.1 Zero-coupon bonds. Price at 0 of a unit bond maturing at T, for an
// implied rate r compounded m times a year (nullopt: continuously).
inline double ZeroCouponPrice(double r, double maturity,
                              std::optional<int> m = std::nullopt) {
  if (!m) return std::exp(-r * maturity);
  return std::pow(1 + r / *m, -maturity * *m);
}

// Rate implied by a unit bond price B with maturity T (inverse of
// ZeroCouponPrice).
inline double ImpliedRate(double bond_price, double maturity,
                          std::optional<int> m = std::nullopt) {
  if (!m) return -std::log(bond_price) / maturity;
  return *m * (std::pow(bond_price, -1 / (maturity * *m)) - 1);
}

// 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F
// paid with the last coupon; y is the yield per period.
//
// The coupons form a geometric series, so
//   price = F v + C (1 - v) / y,   v = (1+y)^-n,
// with 1 - v computed as -expm1(-n log1p(y)) to stay accurate for small y.
inline double CouponBondPrice(double face, double coupon, int n, double y) {
  if (y == 0) return face + coupon * n;
  const double log_v = -n * std::log1p(y);
  return face * std::exp(log_v) - coupon * std::expm1(log_v) / y;
}

// Derivative of CouponBondPrice in y:
//   -(n F v + C sum_k k (1+y)^-k) / (1+y).
inline double CouponBondPriceSlope(double face, double coupon, int n,
                                   double y) {
  const double discount = 1 / (1 + y);
  double factor = 1.0;
  double weighted = 0.0;
  for (int k = 1; k <= n; ++k) {
    factor *= discount;
    weighted += k * factor;
  }
  return -(n * face * factor + coupon * weighted) * discount;
}

// Per-period yield y in [lo, hi] with CouponBondPrice(face, coupon, n, y) =
// price. Newton's method, kept inside a shrinking bisection bracket (the price
// is strictly decreasing in y), so it falls back to bisection whenever a
// Newton step would leave the bracket and returns the nearer end when the
// price is out of range. Stops once the bracket or the Newton step is
// narrower than `tol`. `n` is the number of coupon periods.
inline double YieldToMaturity(double price, double face, double coupon, int n,
                              double lo = -0.99, double hi = 10.0,
                              double tol = 1e-12) {
  // Start from the textbook approximation (C + (F - P) / n) / P.
  double y = (coupon + (face - price) / n) / price;
  if (!(y > lo && y < hi)) y = (lo + hi) / 2;
  for (int iteration = 0; iteration < 200 && hi - lo >= tol; ++iteration) {
    const double excess = CouponBondPrice(face, coupon, n, y) - price;
    if (excess == 0) return y;
    if (excess > 0) {
      lo = y;
    } else {
      hi = y;
    }
    double next = y - excess / CouponBondPriceSlope(face, coupon, n, y);
    if (!(next > lo && next < hi)) next = (lo + hi) / 2;
    if (std::abs(next - y) < tol) return next;
    y = next;
  }
  return (lo + hi) / 2;
}

// 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous
// rate.
inline double MoneyMarketValue(double a0, double r, double t) {
  return a0 * std::exp(r * t);
}

// Money market account built by rolling over short zero-coupon bonds: at each
// step the whole balance buys bonds at unit price B, which pay 1 per bond.
inline double MoneyMarketRollover(double a0,
                                  const std::vector<double>& bond_prices) {
  double balance = a0;
  for (double bond_price : bond_prices) balance /= bond_price;
  return balance;
}

}  // namespace m4f::ch02

#endif  // M4F_CH02_MONEY_MARKET_H_
