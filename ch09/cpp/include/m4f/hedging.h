// Section 9.1: hedging option positions with the stock, the money market and
// other options. A Position is a number of options (negative = written) with a
// given strike, expiry and type; all options are on the same stock, priced by
// Black-Scholes with a common r and sigma.
#ifndef M4F_CH09_HEDGING_H_
#define M4F_CH09_HEDGING_H_

#include <cmath>
#include <cstddef>
#include <vector>

#include "m4f/black_scholes.h"

namespace m4f::ch09 {

struct Position {
  double qty;
  double strike;
  double expiry;
  bool is_call;
};

// Options held, plus shares and money market balance at time 0.
struct Hedge {
  std::vector<Position> options;
  double shares;
  double money;
};

// Value at time t when the stock is at s (money accrues at e^(rt)).
inline double PortfolioValue(const Hedge& h, double s, double t, double r,
                             double sigma) {
  double v = h.shares * s + h.money * std::exp(r * t);
  for (const Position& p : h.options) {
    v += p.qty * BsPrice(s, p.strike, r, sigma, p.expiry - t, p.is_call);
  }
  return v;
}

inline double TotalGreek(const std::vector<Position>& options, double s,
                         double r, double sigma, Greek which) {
  double g = 0.0;
  for (const Position& p : options) {
    g += p.qty *
         GetGreek(BsGreeks(s, p.strike, r, sigma, p.expiry, p.is_call), which);
  }
  return g;
}

inline double Premium(const std::vector<Position>& options, double s, double r,
                      double sigma) {
  double v = 0.0;
  for (const Position& p : options) {
    v += p.qty * BsPrice(s, p.strike, r, sigma, p.expiry, p.is_call);
  }
  return -v;
}

// Section 9.1.1: make the position delta neutral with shares; the money market
// absorbs the premium received and the cost of the shares (value 0 at t=0).
inline Hedge DeltaHedge(const std::vector<Position>& options, double s,
                        double r, double sigma) {
  const double shares = -TotalGreek(options, s, r, sigma, Greek::kDelta);
  return {options, shares, Premium(options, s, r, sigma) - shares * s};
}

// Section 9.1.2: neutralise delta and one more Greek (gamma or vega) with
// shares and a position in a second option `extra` (its qty is ignored and
// solved for).
inline Hedge DeltaGreekHedge(const std::vector<Position>& options,
                             const Position& extra, double s, double r,
                             double sigma, Greek which) {
  const double g_extra = GetGreek(
      BsGreeks(s, extra.strike, r, sigma, extra.expiry, extra.is_call), which);
  const double qty = -TotalGreek(options, s, r, sigma, which) / g_extra;
  std::vector<Position> all = options;
  all.push_back({qty, extra.strike, extra.expiry, extra.is_call});
  const double shares = -TotalGreek(all, s, r, sigma, Greek::kDelta);
  return {all, shares, Premium(all, s, r, sigma) - shares * s};
}

struct RebalanceStep {
  double t;
  double s;
  double shares;
  double money;
  double value;  // Mark-to-market value of the whole hedged position.
};

// Dynamic delta hedging: re-hedge at each price on the path (prices[0] at
// t=0, then one price every `dt` years).
inline std::vector<RebalanceStep> RebalancePath(
    const std::vector<Position>& options, const std::vector<double>& prices,
    double dt, double r, double sigma) {
  std::vector<RebalanceStep> steps;
  const Hedge h = DeltaHedge(options, prices[0], r, sigma);
  double shares = h.shares;
  double money = h.money;
  steps.push_back({0.0, prices[0], shares, money,
                   PortfolioValue(h, prices[0], 0.0, r, sigma)});
  for (std::size_t i = 1; i < prices.size(); ++i) {
    const double t = static_cast<double>(i) * dt;
    const double s = prices[i];
    money = money * std::exp(r * dt);
    std::vector<Position> shifted;
    for (const Position& p : options) {
      shifted.push_back({p.qty, p.strike, p.expiry - t, p.is_call});
    }
    const double new_shares = -TotalGreek(shifted, s, r, sigma, Greek::kDelta);
    money -= (new_shares - shares) * s;
    shares = new_shares;
    double value = shares * s + money;
    for (const Position& p : options) {
      value += p.qty * BsPrice(s, p.strike, r, sigma, p.expiry - t, p.is_call);
    }
    steps.push_back({t, s, shares, money, value});
  }
  return steps;
}

}  // namespace m4f::ch09

#endif  // M4F_CH09_HEDGING_H_
