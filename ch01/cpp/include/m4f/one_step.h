// Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
// "Introduction: A Simple Market Model" -- the one-step binomial market.
//
// Two assets, two time instants t = 0, 1:
//   risk-free bond  A(0) -> A(1)                     (return r)
//   risky stock     S(0) -> S^u(1) with prob. p      (return u)
//                         S^d(1) with prob. 1 - p    (return d)
// A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.
//
// Names follow the book's notation, lower-cased for the style guide:
// a0 = A(0), a1 = A(1), s0 = S(0), s_up = S^u(1), s_down = S^d(1).
#ifndef M4F_CH01_ONE_STEP_H_
#define M4F_CH01_ONE_STEP_H_

#include <cmath>
#include <optional>

#include "m4f/check.h"

namespace m4f::ch01 {

enum class State { kUp, kDown };

struct Portfolio {
  double x = 0.0;  // Shares of stock.
  double y = 0.0;  // Units of bond.
};

class OneStepMarket {
 public:
  // Aborts unless all prices are positive, s_up > s_down and 0 < p < 1.
  OneStepMarket(double a0, double a1, double s0, double s_up, double s_down,
                double p)
      : a0_(a0), a1_(a1), s0_(s0), s_up_(s_up), s_down_(s_down), p_(p) {
    Check(a0 > 0 && a1 > 0 && s0 > 0 && s_down > 0 && s_up > s_down,
          "require positive prices and S^u(1) > S^d(1)");
    Check(p > 0 && p < 1, "require 0 < p < 1");
  }

  double a0() const { return a0_; }
  double a1() const { return a1_; }
  double s0() const { return s0_; }
  double s1(State s) const { return s == State::kUp ? s_up_ : s_down_; }
  double p() const { return p_; }

  // Returns K = (P(1) - P(0)) / P(0) for the bond (r) and the stock (u, d).
  double r() const { return (a1_ - a0_) / a0_; }
  double u() const { return (s_up_ - s0_) / s0_; }
  double d() const { return (s_down_ - s0_) / s0_; }

  // Theorem: no arbitrage <=> d < r < u.
  bool IsArbitrageFree() const { return d() < r() && r() < u(); }

  // Risk-neutral probability p* = (r - d) / (u - d); makes
  // E*[S(1)] = (1 + r) S(0).
  double RiskNeutralProb() const { return (r() - d()) / (u() - d()); }

  // Portfolio investing `budget` with fraction `w` in stock, the rest in
  // bonds.
  Portfolio Split(double budget, double w) const {
    return {w * budget / s0_, (1 - w) * budget / a0_};
  }

  double Value0(const Portfolio& portfolio) const {
    return portfolio.x * s0_ + portfolio.y * a0_;
  }
  double Value1(const Portfolio& portfolio, State s) const {
    return portfolio.x * s1(s) + portfolio.y * a1_;
  }
  double PortfolioReturn(const Portfolio& portfolio, State s) const {
    return (Value1(portfolio, s) - Value0(portfolio)) / Value0(portfolio);
  }

  // Expectation and standard deviation of a two-point random variable
  // taking values `up` (prob. p) and `down` (prob. 1 - p).
  double Expect(double up, double down) const {
    return p_ * up + (1 - p_) * down;
  }
  double StdDev(double up, double down) const {
    return std::sqrt(p_ * (1 - p_)) * std::abs(up - down);
  }

  // Replicating portfolio for a derivative paying `d_up` / `d_down` at t=1:
  //   x S^u(1) + y A(1) = D^u,   x S^d(1) + y A(1) = D^d
  Portfolio Replicate(double d_up, double d_down) const {
    const double x = (d_up - d_down) / (s_up_ - s_down_);
    const double y = (d_up - x * s_up_) / a1_;
    return {x, y};
  }

  // No-arbitrage price D(0) = value of the replicating portfolio ...
  double Price(double d_up, double d_down) const {
    return Value0(Replicate(d_up, d_down));
  }

  // ... which equals the discounted risk-neutral expectation.
  double RiskNeutralPrice(double d_up, double d_down) const {
    const double q = RiskNeutralProb();
    return (q * d_up + (1 - q) * d_down) / (1 + r());
  }

  // Forward price for delivery at t=1: F = S(0) (1 + r).
  double ForwardPrice() const { return s0_ * (1 + r()); }

  // If the model admits arbitrage, returns a portfolio with V(0) = 0,
  // V(1) >= 0 and V(1) > 0 in some state, using one share; otherwise nullopt.
  std::optional<Portfolio> Arbitrage() const {
    // Short one share and buy bonds with the proceeds.
    if (r() >= u()) return Portfolio{-1.0, s0_ / a0_};
    // Borrow to buy one share.
    if (r() <= d()) return Portfolio{1.0, -s0_ / a0_};
    return std::nullopt;
  }

 private:
  double a0_;
  double a1_;
  double s0_;
  double s_up_;
  double s_down_;
  double p_;
};

inline double CallPayoff(double s, double strike) {
  return s > strike ? s - strike : 0.0;
}
inline double PutPayoff(double s, double strike) {
  return strike > s ? strike - s : 0.0;
}

// Section 1.7: cost at t=1 of buying one share for an investor who at t=0
// borrowed to buy `num_calls` calls with strike `strike`:
//   S(1) - n C(1) + n C(0) (1 + r).
inline double HedgedPurchaseCost(const OneStepMarket& market, double strike,
                                 double num_calls, State s) {
  const double call0 =
      market.Price(CallPayoff(market.s1(State::kUp), strike),
                   CallPayoff(market.s1(State::kDown), strike));
  return market.s1(s) - num_calls * CallPayoff(market.s1(s), strike) +
         num_calls * call0 * (1 + market.r());
}

}  // namespace m4f::ch01

#endif  // M4F_CH01_ONE_STEP_H_
