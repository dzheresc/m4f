// Section 10.2: general term structure. The market gives zero-coupon bond
// prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
// continuously compounded. In a deterministic term structure the implied
// forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).
#ifndef M4F_CH10_TERM_STRUCTURE_H_
#define M4F_CH10_TERM_STRUCTURE_H_

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

// Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid.
inline double CouponBondPrice(const std::vector<double>& flow_times,
                              const std::vector<double>& flow_amounts,
                              const std::vector<double>& maturities,
                              const std::vector<double>& prices) {
  double total = 0.0;
  for (std::size_t i = 0; i < flow_times.size(); ++i) {
    for (std::size_t j = 0; j < maturities.size(); ++j) {
      if (maturities[j] == flow_times[i]) {
        total += flow_amounts[i] * prices[j];
        break;
      }
    }
  }
  return total;
}

// The single continuous yield y with sum C_i e^(-y t_i) = P (bisection).
inline double YieldToMaturity(double price,
                              const std::vector<double>& flow_times,
                              const std::vector<double>& flow_amounts) {
  double lo = -1.0;
  double hi = 1.0;
  for (int it = 0; it < 200; ++it) {
    const double mid = (lo + hi) / 2;
    double s = 0.0;
    for (std::size_t i = 0; i < flow_times.size(); ++i) {
      s += flow_amounts[i] * std::exp(-mid * flow_times[i]);
    }
    if (s > price) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return (lo + hi) / 2;
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
