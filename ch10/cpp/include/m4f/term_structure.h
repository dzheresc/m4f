// Section 10.2: general term structure. The market gives zero-coupon bond
// prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
// continuously compounded. In a deterministic term structure the implied
// forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).
#ifndef M4F_CH10_TERM_STRUCTURE_H_
#define M4F_CH10_TERM_STRUCTURE_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch10 {

// y(0,T) = -ln B(0,T) / T.
inline double YieldRate(double bond_price, double maturity) {
  return -std::log(bond_price) / maturity;
}

// f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1); B(0,0) = 1 gives
// y(0,T2).
inline double ForwardRate(double b1, double t1, double b2, double t2) {
  return (std::log(b1) - std::log(b2)) / (t2 - t1);
}

// f(0,T_{i-1},T_i) for each i, with T_0 = 0.
inline std::vector<double> ForwardRates(const std::vector<double>& maturities,
                                        const std::vector<double>& prices) {
  std::vector<double> out;
  double b_prev = 1.0;
  double t_prev = 0.0;
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    out.push_back(ForwardRate(b_prev, t_prev, prices[i], maturities[i]));
    b_prev = prices[i];
    t_prev = maturities[i];
  }
  return out;
}

// B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1})).
inline std::vector<double> PricesFromForwardRates(
    const std::vector<double>& maturities, const std::vector<double>& f) {
  std::vector<double> out;
  double acc = 0.0;
  double t_prev = 0.0;
  for (std::size_t i = 0; i < maturities.size(); ++i) {
    acc += f[i] * (maturities[i] - t_prev);
    out.push_back(std::exp(-acc));
    t_prev = maturities[i];
  }
  return out;
}

// B(t,T) implied by no arbitrage in a deterministic term structure:
// B(0,T)/B(0,t).
inline double ImpliedPrice(double b_t, double b_maturity) {
  return b_maturity / b_t;
}

// Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid (matched
// exactly). The maturities are ascending, so sorted flow times are matched in
// one merge pass; a flow time earlier than the one before restarts the scan.
inline double CouponBondPrice(const std::vector<double>& flow_times,
                              const std::vector<double>& flow_amounts,
                              const std::vector<double>& maturities,
                              const std::vector<double>& prices) {
  double total = 0.0;
  std::size_t j = 0;
  for (std::size_t i = 0; i < flow_times.size(); ++i) {
    if (i > 0 && flow_times[i] < flow_times[i - 1]) j = 0;
    while (j < maturities.size() && maturities[j] < flow_times[i]) ++j;
    if (j < maturities.size() && maturities[j] == flow_times[i]) {
      total += flow_amounts[i] * prices[j];
    }
  }
  return total;
}

// The single continuous yield y in [-1, 1] with sum C_i e^(-y t_i) = P.
// Newton's method (the derivative is minus the duration sum, from the same
// loop) inside a bisection bracket: a step that leaves the bracket is
// replaced by its midpoint, so it converges wherever bisection would.
inline double YieldToMaturity(double price,
                              const std::vector<double>& flow_times,
                              const std::vector<double>& flow_amounts) {
  double lo = -1.0;
  double hi = 1.0;
  double total = 0.0;
  double weighted = 0.0;
  for (std::size_t i = 0; i < flow_times.size(); ++i) {
    total += flow_amounts[i];
    weighted += flow_amounts[i] * flow_times[i];
  }
  // Starting guess: the yield if all flows were paid at their mean time.
  double y = total > 0 && price > 0 && weighted > 0
                 ? std::log(total / price) * total / weighted
                 : 0.0;
  if (!(y > lo && y < hi)) y = 0.0;
  for (int it = 0; it < 200; ++it) {
    double s = 0.0;
    double ds = 0.0;  // -d s / dy.
    for (std::size_t i = 0; i < flow_times.size(); ++i) {
      const double term = flow_amounts[i] * std::exp(-y * flow_times[i]);
      s += term;
      ds += flow_times[i] * term;
    }
    if (s > price) {
      lo = y;
    } else {
      hi = y;
    }
    if (ds > 0) {
      const double step = (s - price) / ds;
      // Converged: y is the root to within rounding, and may sit on a
      // bracket end, so test this before the bracket.
      if (std::abs(step) <= 1e-15 * std::max(1.0, std::abs(y))) {
        return y + step;
      }
      y += step;
    }
    if (!(y > lo && y < hi)) y = (lo + hi) / 2;
    if (hi - lo <= 1e-15) return y;
  }
  return y;
}

// Section 10.2.2: A(T_n) from rolling over one-period bonds at the (implied,
// hence realised) short rates: A(T_n) = prod 1/B(T_{i-1},T_i) = 1/B(0,T_n).
inline std::vector<double> MoneyMarketAccount(
    const std::vector<double>& /*maturities*/,
    const std::vector<double>& prices) {
  std::vector<double> out;
  double a = 1.0;
  double b_prev = 1.0;
  for (double b : prices) {
    a = a / ImpliedPrice(b_prev, b);
    out.push_back(a);
    b_prev = b;
  }
  return out;
}

}  // namespace m4f::ch10

#endif  // M4F_CH10_TERM_STRUCTURE_H_
