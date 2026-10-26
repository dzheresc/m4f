// Section 2.2, money market: zero-coupon bonds, coupon bonds and the money
// market account. Bond prices are per unit face value unless F is given.
#pragma once

#include <cmath>
#include <optional>
#include <vector>

namespace m4f::ch02 {

// 2.2.1 Zero-coupon bonds. Price at 0 of a unit bond maturing at T, for an implied
// rate r compounded m times a year (nullopt: continuously).
inline double zeroCouponPrice(double r, double T, std::optional<int> m = std::nullopt) {
    return m ? std::pow(1 + r / *m, -T * *m) : std::exp(-r * T);
}
// Rate implied by a unit bond price B with maturity T (inverse of zeroCouponPrice).
inline double impliedRate(double B, double T, std::optional<int> m = std::nullopt) {
    return m ? *m * (std::pow(B, -1 / (T * *m)) - 1) : -std::log(B) / T;
}

// 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F paid
// with the last coupon; y is the yield per period.
inline double couponBondPrice(double F, double C, int n, double y) {
    double price = F * std::pow(1 + y, -n);
    for (int k = 1; k <= n; ++k) price += C * std::pow(1 + y, -k);
    return price;
}
// Per-period yield y with couponBondPrice(F, C, n, y) = price (bisection; the
// price is strictly decreasing in y).
inline double yieldToMaturity(double price, double F, double C, int n, double lo = -0.99,
                              double hi = 10.0, double tol = 1e-12) {
    for (int it = 0; it < 200 && hi - lo >= tol; ++it) {
        const double mid = (lo + hi) / 2;
        (couponBondPrice(F, C, n, mid) > price ? lo : hi) = mid;
    }
    return (lo + hi) / 2;
}

// 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous rate.
inline double moneyMarketValue(double A0, double r, double t) { return A0 * std::exp(r * t); }

// Money market account built by rolling over short zero-coupon bonds: at each
// step the whole balance buys bonds at unit price B, which pay 1 per bond.
inline double moneyMarketRollover(double A0, const std::vector<double>& prices) {
    double A = A0;
    for (double B : prices) A /= B;
    return A;
}

}  // namespace m4f::ch02
