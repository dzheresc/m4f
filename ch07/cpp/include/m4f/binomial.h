// Option prices used to illustrate Sections 7.3-7.5: the Cox-Ross-Rubinstein
// tree with N steps of length tau = T/N, u = e^(sigma sqrt(tau)), d = 1/u,
// growth e^(r tau) per step (Chapter 8 develops the model itself).
#ifndef M4F_CH07_BINOMIAL_H_
#define M4F_CH07_BINOMIAL_H_

#include <algorithm>
#include <cmath>
#include <vector>

namespace m4f::ch07 {

inline double CrrPrice(double s0, double strike, double r, double sigma,
                       double maturity, int num_steps, bool is_call,
                       bool american = false) {
  const double tau = maturity / num_steps;
  const double up = std::exp(sigma * std::sqrt(tau));
  const double down = 1 / up;
  const double growth = std::exp(r * tau);
  const double q = (growth - down) / (up - down);
  const auto payoff = [&](double s) {
    return is_call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0);
  };

  std::vector<double> values;
  for (int k = 0; k <= num_steps; ++k) {
    values.push_back(
        payoff(s0 * std::pow(up, k) * std::pow(down, num_steps - k)));
  }
  for (int n = num_steps - 1; n >= 0; --n) {
    for (int k = 0; k <= n; ++k) {
      double v = (q * values[k + 1] + (1 - q) * values[k]) / growth;
      if (american) {
        v = std::max(v, payoff(s0 * std::pow(up, k) * std::pow(down, n - k)));
      }
      values[k] = v;
    }
  }
  return values[0];
}

}  // namespace m4f::ch07

#endif  // M4F_CH07_BINOMIAL_H_
