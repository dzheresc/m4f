// Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree
// that converges to it (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma sqrt(tau)),
// 1+r = e^(r tau), tau = T/N). r is the continuously compounded rate.
#ifndef M4F_CH08_BLACK_SCHOLES_H_
#define M4F_CH08_BLACK_SCHOLES_H_

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "m4f/binomial_pricing.h"

namespace m4f::ch08 {

// Standard normal CDF to double precision (Hart 1968, as given by G. West,
// "Better approximations to cumulative normal functions").
inline double NormCdf(double x) {
  const double a = std::abs(x);
  double c;
  if (a > 37.0) {
    c = 0.0;
  } else {
    const double e = std::exp(-a * a / 2);
    double b;
    if (a < 7.07106781186547) {
      b = 3.52624965998911e-02 * a + 0.700383064443688;
      b = b * a + 6.37396220353165;
      b = b * a + 33.912866078383;
      b = b * a + 112.079291497871;
      b = b * a + 221.213596169931;
      b = b * a + 220.206867912376;
      c = e * b;
      b = 8.83883476483184e-02 * a + 1.75566716318264;
      b = b * a + 16.064177579207;
      b = b * a + 86.7807322029461;
      b = b * a + 296.564248779674;
      b = b * a + 637.333633378831;
      b = b * a + 793.826512519948;
      b = b * a + 440.413735824752;
      c = c / b;
    } else {
      b = a + 0.65;
      b = a + 4 / b;
      b = a + 3 / b;
      b = a + 2 / b;
      b = a + 1 / b;
      c = e / b / 2.506628274631;
    }
  }
  return x > 0 ? 1 - c : c;
}

// (d1, d2) with d1 = (ln(S/X) + (r + sigma^2/2) T) / (sigma sqrt(T)) and
// d2 = d1 - sigma sqrt(T).
inline std::pair<double, double> D1D2(double s, double strike, double r,
                                      double sigma, double maturity) {
  const double d1 =
      (std::log(s / strike) + (r + sigma * sigma / 2) * maturity) /
      (sigma * std::sqrt(maturity));
  return {d1, d1 - sigma * std::sqrt(maturity)};
}

// C = S N(d1) - X e^(-rT) N(d2).
inline double BsCall(double s, double strike, double r, double sigma,
                     double maturity) {
  const auto [d1, d2] = D1D2(s, strike, r, sigma, maturity);
  return s * NormCdf(d1) - strike * std::exp(-r * maturity) * NormCdf(d2);
}

// P = X e^(-rT) N(-d2) - S N(-d1).
inline double BsPut(double s, double strike, double r, double sigma,
                    double maturity) {
  const auto [d1, d2] = D1D2(s, strike, r, sigma, maturity);
  return strike * std::exp(-r * maturity) * NormCdf(-d2) - s * NormCdf(-d1);
}

// Shares in the replicating portfolio: N(d1).
inline double BsCallDelta(double s, double strike, double r, double sigma,
                          double maturity) {
  return NormCdf(D1D2(s, strike, r, sigma, maturity).first);
}

inline double CrrTreePrice(double s, double strike, double r, double sigma,
                           double maturity, int num_steps, bool is_call,
                           bool american = false) {
  const double tau = maturity / num_steps;
  const double u = std::exp(sigma * std::sqrt(tau)) - 1;
  const double d = std::exp(-sigma * std::sqrt(tau)) - 1;
  const double r_step = std::exp(r * tau) - 1;
  // The same backward induction as OptionTree, keeping one row instead of
  // the whole tree and calling the payoff directly.
  const double q = RiskNeutralProb(u, d, r_step);
  const StockPrices price(s, u, d, num_steps);
  const auto payoff = [strike, is_call](double x) {
    return is_call ? std::max(x - strike, 0.0) : std::max(strike - x, 0.0);
  };
  std::vector<double> values(num_steps + 1);
  for (int k = 0; k <= num_steps; ++k) values[k] = payoff(price(num_steps, k));
  for (int n = num_steps - 1; n >= 0; --n) {
    for (int k = 0; k <= n; ++k) {
      double v = (q * values[k + 1] + (1 - q) * values[k]) / (1 + r_step);
      if (american) v = std::max(v, payoff(price(n, k)));
      values[k] = v;
    }
  }
  return values[0];
}

}  // namespace m4f::ch08

#endif  // M4F_CH08_BLACK_SCHOLES_H_
