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

  // With down = 1/up the stock price at node (n, k) is s0 up^(2k - n), so all
  // node prices come from one table of 2N+1 powers instead of two std::pow
  // calls per node.
  std::vector<double> prices(2 * num_steps + 1);
  for (int j = 0; j <= 2 * num_steps; ++j) {
    prices[j] = s0 * std::pow(up, j - num_steps);
  }
  const auto price = [&](int n, int k) {
    return prices[2 * k - n + num_steps];
  };
  std::vector<double> values(num_steps + 1);
  for (int k = 0; k <= num_steps; ++k) values[k] = payoff(price(num_steps, k));
  for (int n = num_steps - 1; n >= 0; --n) {
    for (int k = 0; k <= n; ++k) {
      double v = (q * values[k + 1] + (1 - q) * values[k]) / growth;
      if (american) v = std::max(v, payoff(price(n, k)));
      values[k] = v;
    }
  }
  return values[0];
}

}  // namespace m4f::ch07

#endif  // M4F_CH07_BINOMIAL_H_
