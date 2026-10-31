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
inline double CouponBondPrice(double face, double coupon, int n, double y) {
  double price = face * std::pow(1 + y, -n);
  for (int k = 1; k <= n; ++k) price += coupon * std::pow(1 + y, -k);
  return price;
}

// Per-period yield y with CouponBondPrice(face, coupon, n, y) = price, found by
// bisection on [lo, hi] (the price is strictly decreasing in y).
inline double YieldToMaturity(double price, double face, double coupon, int n,
                              double lo = -0.99, double hi = 10.0,
                              double tol = 1e-12) {
  for (int iteration = 0; iteration < 200 && hi - lo >= tol; ++iteration) {
    const double mid = (lo + hi) / 2;
    if (CouponBondPrice(face, coupon, n, mid) > price) {
      lo = mid;
    } else {
      hi = mid;
    }
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
