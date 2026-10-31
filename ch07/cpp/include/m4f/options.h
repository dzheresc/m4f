// Sections 7.1-7.3: option payoffs, put-call parity and bounds on option
// prices. Continuous compounding at rate r; `maturity` is the time T to expiry
// in years and `strike` the strike X. `div` is the present value of dividends
// paid before T.
#ifndef M4F_CH07_OPTIONS_H_
#define M4F_CH07_OPTIONS_H_

#include <algorithm>
#include <cmath>

namespace m4f::ch07 {

// Section 7.1: definitions.
inline double CallPayoff(double s, double strike) {
  return std::max(s - strike, 0.0);
}

inline double PutPayoff(double s, double strike) {
  return std::max(strike - s, 0.0);
}

// Profit at T of a long position bought for `premium` at 0 (premium financed
// at r); a short position earns the negative of it.
inline double Profit(double payoff, double premium, double r, double maturity) {
  return payoff - premium * std::exp(r * maturity);
}

// Section 7.2: put-call parity, C - P = S(0) - div - X e^(-rT).
// Returns C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy the
// put and the stock, borrow; negative: the reverse. Either locks in
// |gap| e^(rT) at T.
inline double ParityGap(double call, double put, double s0, double strike,
                        double r, double maturity, double div = 0.0) {
  return call - put - (s0 - div - strike * std::exp(-r * maturity));
}

struct Bounds {
  double lo;
  double hi;
};

// Section 7.3.1: European options.
// max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div.
inline Bounds EuropeanCallBounds(double s0, double strike, double r,
                                 double maturity, double div = 0.0) {
  return {std::max(0.0, s0 - div - strike * std::exp(-r * maturity)), s0 - div};
}

// max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT).
inline Bounds EuropeanPutBounds(double s0, double strike, double r,
                                double maturity, double div = 0.0) {
  return {std::max(0.0, strike * std::exp(-r * maturity) - s0 + div),
          strike * std::exp(-r * maturity)};
}

// How far a quoted price lies outside [lo, hi] (0 if inside); this much can be
// locked in at time 0 by trading against the quote.
inline double BoundViolation(double price, Bounds b) {
  return price < b.lo ? b.lo - price : (price > b.hi ? price - b.hi : 0.0);
}

// Section 7.3.3: American options,
// S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends).
inline Bounds AmericanParityBounds(double s0, double strike, double r,
                                   double maturity) {
  return {s0 - strike, s0 - strike * std::exp(-r * maturity)};
}

// Section 7.4.1: dependence on the strike, for X1 < X2 < X3. Convexity in X:
// C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1). A positive return value
// is the arbitrage profit from writing the middle option and buying the wings.
inline double ButterflyGap(double c1, double c2, double c3, double x1,
                           double x2, double x3) {
  const double lam = (x3 - x2) / (x3 - x1);
  return c2 - (lam * c1 + (1 - lam) * c3);
}

}  // namespace m4f::ch07

#endif  // M4F_CH07_OPTIONS_H_
