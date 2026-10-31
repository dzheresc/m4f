// Section 10.1: maturity-independent yields. All bonds are priced with one
// continuously compounded yield y, so B(t,T) = e^(-y(T-t)). A Bond is a list
// of cash flows (t_i, C_i); coupons and the face value are flows.
#ifndef M4F_CH10_BONDS_H_
#define M4F_CH10_BONDS_H_

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace m4f::ch10 {

struct Flow {
  double t;
  double amount;
};

using Bond = std::vector<Flow>;

inline Bond ZeroCoupon(double face, double maturity) {
  return {{maturity, face}};
}

// n coupons of `coupon` at dt, 2dt, ..., n dt; the face is repaid with the
// last.
inline Bond CouponBond(double face, double coupon, int n, double dt = 1.0) {
  Bond b;
  for (int i = 0; i < n; ++i) {
    b.push_back({(i + 1) * dt, coupon + (i == n - 1 ? face : 0.0)});
  }
  return b;
}

// P(t) = sum over t_i > t of C_i e^(-y(t_i - t)).
inline double Price(const Bond& bond, double y, double t = 0.0) {
  double p = 0.0;
  for (const Flow& flow : bond) {
    if (flow.t > t) p += flow.amount * std::exp(-y * (flow.t - t));
  }
  return p;
}

// D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P.
inline double Duration(const Bond& bond, double y, double t = 0.0) {
  double s = 0.0;
  for (const Flow& flow : bond) {
    if (flow.t > t)
      s += (flow.t - t) * flow.amount * std::exp(-y * (flow.t - t));
  }
  return s / Price(bond, y, t);
}

// Section 10.1.1: value at time t of one bond bought at 0, if the yield is y
// from then on: coupons received before t are reinvested at y, the rest is
// priced at y.
inline double InvestmentValue(const Bond& bond, double y, double t) {
  double v = 0.0;
  for (const Flow& flow : bond) {
    v += flow.t <= t ? flow.amount * std::exp(y * (t - flow.t))
                     : flow.amount * std::exp(-y * (flow.t - t));
  }
  return v;
}

// Annualised log return over [0,t] when buying at yield y0 and the yield
// jumps to y.
inline double LogReturn(const Bond& bond, double y0, double y, double t) {
  return std::log(InvestmentValue(bond, y, t) / Price(bond, y0)) / t;
}

// Section 10.1.3: portfolios of two bonds. The duration of a portfolio is the
// value-weighted average of durations. Weights w1 + w2 = 1 with
// w1 D1 + w2 D2 = D_target.
inline std::pair<double, double> ImmunisingWeights(double d1, double d2,
                                                   double d_target) {
  const double w1 = (d_target - d2) / (d1 - d2);
  return {w1, 1 - w1};
}

struct Portfolio {
  double q1;  // Number of bond 1.
  double q2;  // Number of bond 2.
  double cash;
};

inline double PortfolioValue(const Portfolio& p, const Bond& b1, const Bond& b2,
                             double y, double t) {
  return p.q1 * Price(b1, y, t) + p.q2 * Price(b2, y, t) + p.cash;
}

// Invest v at time t in bonds 1 and 2 so that the duration equals
// horizon - t.
inline Portfolio Immunise(double v, const Bond& b1, const Bond& b2, double y,
                          double t, double horizon) {
  const auto [w1, w2] =
      ImmunisingWeights(Duration(b1, y, t), Duration(b2, y, t), horizon - t);
  return {w1 * v / Price(b1, y, t), w2 * v / Price(b2, y, t), 0.0};
}

struct HedgeStep {
  double t;
  double y;
  double value;
  double q1;
  double q2;
  double duration;  // Of the rebalanced portfolio.
};

// Section 10.1.4: along a path of (t, y) with t[0] = 0, hold the two bonds,
// collect coupons into cash (accruing at the yield of the interval), and, if
// `rebalance`, re-immunise at each step so the duration equals the remaining
// horizon.
inline std::vector<HedgeStep> DynamicHedge(
    double v0, const Bond& b1, const Bond& b2, double horizon,
    const std::vector<std::pair<double, double>>& path, bool rebalance = true) {
  const auto [t0, y0] = path[0];
  Portfolio p = Immunise(v0, b1, b2, y0, t0, horizon);
  std::vector<HedgeStep> steps = {{t0, y0, v0, p.q1, p.q2, horizon - t0}};
  for (std::size_t i = 1; i < path.size(); ++i) {
    const auto [t_prev, y_prev] = path[i - 1];
    const auto [t, y] = path[i];
    double cash = p.cash * std::exp(y_prev * (t - t_prev));
    const std::pair<double, const Bond*> holdings[] = {{p.q1, &b1},
                                                       {p.q2, &b2}};
    for (const auto& [q, b] : holdings) {
      for (const Flow& flow : *b) {
        if (t_prev < flow.t && flow.t <= t) {
          cash += q * flow.amount * std::exp(y_prev * (t - flow.t));
        }
      }
    }
    p = {p.q1, p.q2, cash};
    const double v = PortfolioValue(p, b1, b2, y, t);
    if (rebalance && t < horizon) p = Immunise(v, b1, b2, y, t, horizon);
    const double d = t < horizon
                         ? (p.q1 * Price(b1, y, t) * Duration(b1, y, t) +
                            p.q2 * Price(b2, y, t) * Duration(b2, y, t)) /
                               v
                         : 0.0;
    steps.push_back({t, y, v, p.q1, p.q2, d});
  }
  return steps;
}

}  // namespace m4f::ch10

#endif  // M4F_CH10_BONDS_H_
