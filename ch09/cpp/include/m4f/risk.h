// Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock,
//     S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
// so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
// expected logarithmic return; the Black-Scholes r only enters option prices.
//
// A Strategy is shares + cash + long/short options held to T. Its terminal
// value V(S(T)) is assumed non-decreasing in S(T) (long stock, long calls, long
// puts hedging stock, bull spreads), so quantiles of V are V at quantiles of
// S(T).
#ifndef M4F_CH09_RISK_H_
#define M4F_CH09_RISK_H_

#include <cmath>
#include <string>
#include <vector>

#include "m4f/black_scholes.h"

namespace m4f::ch09 {

// The lognormal model of S(T).
struct Lognormal {
  double s0;
  double m;
  double sigma;
  double maturity;
};

// The p-quantile of S(T).
inline double Quantile(const Lognormal& model, double p) {
  return model.s0 *
         std::exp(model.m * model.maturity +
                  model.sigma * std::sqrt(model.maturity) * NormInv(p));
}

// P(S(T) <= s).
inline double Cdf(const Lognormal& model, double s) {
  return NormCdf((std::log(s / model.s0) - model.m * model.maturity) /
                 (model.sigma * std::sqrt(model.maturity)));
}

// E S(T).
inline double ExpectedStock(const Lognormal& model) {
  return model.s0 * std::exp(model.m * model.maturity +
                             model.sigma * model.sigma * model.maturity / 2);
}

// E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2).
inline double ExpectedCallPayoff(const Lognormal& model, double strike) {
  const double sq = model.sigma * std::sqrt(model.maturity);
  const double delta2 =
      (std::log(model.s0 / strike) + model.m * model.maturity) / sq;
  const double delta1 = delta2 + sq;
  return ExpectedStock(model) * NormCdf(delta1) - strike * NormCdf(delta2);
}

// E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X.
inline double ExpectedPutPayoff(const Lognormal& model, double strike) {
  return ExpectedCallPayoff(model, strike) - ExpectedStock(model) + strike;
}

struct OptionLeg {
  double qty;
  double strike;
  bool is_call;
};

struct Strategy {
  std::string name;
  double shares;
  double cash;  // Invested in the money market at r.
  std::vector<OptionLeg> legs;
};

// Value at T of the strategy when the stock ends at s.
inline double TerminalValue(const Strategy& st, double s, double r,
                            double maturity) {
  double v = st.shares * s + st.cash * std::exp(r * maturity);
  for (const OptionLeg& leg : st.legs) {
    v += leg.qty * Payoff(s, leg.strike, leg.is_call);
  }
  return v;
}

// E V(T) under the lognormal model.
inline double ExpectedValue(const Strategy& st, const Lognormal& model,
                            double r) {
  double v =
      st.shares * ExpectedStock(model) + st.cash * std::exp(r * model.maturity);
  for (const OptionLeg& leg : st.legs) {
    v += leg.qty * (leg.is_call ? ExpectedCallPayoff(model, leg.strike)
                                : ExpectedPutPayoff(model, leg.strike));
  }
  return v;
}

inline double InitialCost(const Strategy& st, double s0, double r, double sigma,
                          double maturity) {
  double c = st.shares * s0 + st.cash;
  for (const OptionLeg& leg : st.legs) {
    c += leg.qty * BsPrice(s0, leg.strike, r, sigma, maturity, leg.is_call);
  }
  return c;
}

// Section 9.2.1: VaR at confidence level conf = initial value minus the
// (1-conf) quantile of the terminal value (positive = a loss).
inline double ValueAtRisk(const Strategy& st, const Lognormal& model, double r,
                          double sigma, double conf) {
  const double v0 = InitialCost(st, model.s0, r, sigma, model.maturity);
  return v0 - TerminalValue(st, Quantile(model, 1 - conf), r, model.maturity);
}

inline double ExpectedReturn(const Strategy& st, const Lognormal& model,
                             double r, double sigma) {
  const double v0 = InitialCost(st, model.s0, r, sigma, model.maturity);
  return ExpectedValue(st, model, r) / v0 - 1;
}

// Stock price at T where the strategy neither gains nor loses (bisection).
inline double Breakeven(const Strategy& st, const Lognormal& model, double r,
                        double sigma) {
  const double v0 = InitialCost(st, model.s0, r, sigma, model.maturity);
  double lo = 0.0;
  double hi = 100.0 * model.s0;
  for (int i = 0; i < 200; ++i) {
    const double mid = (lo + hi) / 2;
    if (TerminalValue(st, mid, r, model.maturity) < v0) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return (lo + hi) / 2;
}

inline double ProbLoss(const Strategy& st, const Lognormal& model, double r,
                       double sigma) {
  return Cdf(model, Breakeven(st, model, r, sigma));
}

// Section 9.2: a holding of `shares` shares, hedged by buying `h` puts per
// share at the given strike.
inline Strategy PutHedgedHolding(const std::string& name, double shares,
                                 double strike, double h = 1.0) {
  return {name, shares, 0.0, {{h * shares, strike, false}}};
}

inline Strategy UnhedgedHolding(double shares) {
  return {"stock", shares, 0.0, {}};
}

// Section 9.3.1: spend capital v0 on one instrument at stock price s0 = S(0).
inline Strategy AllInStock(double v0, double s0) {
  return {"stock", v0 / s0, 0.0, {}};
}

inline Strategy AllInCalls(const std::string& name, double v0, double s0,
                           double strike, double r, double sigma,
                           double maturity) {
  const double n = v0 / BsPrice(s0, strike, r, sigma, maturity, true);
  return {name, 0.0, 0.0, {{n, strike, true}}};
}

// Buys calls at strike x1 and writes as many at x2 (x1 < x2), as many as v0
// pays for.
inline Strategy BullSpread(const std::string& name, double v0, double s0,
                           double x1, double x2, double r, double sigma,
                           double maturity) {
  const double n = v0 / (BsPrice(s0, x1, r, sigma, maturity, true) -
                         BsPrice(s0, x2, r, sigma, maturity, true));
  return {name, 0.0, 0.0, {{n, x1, true}, {-n, x2, true}}};
}

// Holds strategy `a` scaled by `wa` plus strategy `b` scaled by `wb`.
inline Strategy Mix(const std::string& name, const Strategy& a, double wa,
                    const Strategy& b, double wb) {
  std::vector<OptionLeg> legs;
  for (const OptionLeg& l : a.legs) {
    legs.push_back({l.qty * wa, l.strike, l.is_call});
  }
  for (const OptionLeg& l : b.legs) {
    legs.push_back({l.qty * wb, l.strike, l.is_call});
  }
  return {name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs};
}

}  // namespace m4f::ch09

#endif  // M4F_CH09_RISK_H_
