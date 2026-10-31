// Sections 8.1-8.2: European and American options in the binomial tree model.
//
// The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a
// money market growing at r per step, and p* = (r - d) / (u - d). Payoffs are
// functions of the final stock price.
#ifndef M4F_CH08_BINOMIAL_PRICING_H_
#define M4F_CH08_BINOMIAL_PRICING_H_

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace m4f::ch08 {

using Payoff = std::function<double(double)>;
using Tree = std::vector<std::vector<double>>;

inline double RiskNeutralProb(double u, double d, double r) {
  return (r - d) / (u - d);
}

inline double StockPrice(double s0, double u, double d, int n, int k) {
  return s0 * std::pow(1 + u, k) * std::pow(1 + d, n - k);
}

// Section 8.1.1: one step.
struct Replication {
  double x;      // Shares.
  double y;      // Units of the money market (A(0) = 1).
  double price;  // x S(0) + y.
};

// Replicates the payoff c_up / c_down; price = x S(0) + y.
inline Replication OneStepReplication(double s0, double u, double d, double r,
                                      double c_up, double c_down) {
  const double x = (c_up - c_down) / (s0 * (u - d));
  const double y = (c_down - x * s0 * (1 + d)) / (1 + r);
  return {x, y, x * s0 + y};
}

// S(0) (1+u)^k (1+d)^(n-k) for 0 <= k <= n <= N, read from two tables of
// powers built once: the same arithmetic as StockPrice without two std::pow
// calls per node.
class StockPrices {
 public:
  StockPrices(double s0, double u, double d, int num_steps) : s0_(s0) {
    up_.reserve(num_steps + 1);
    down_.reserve(num_steps + 1);
    for (int j = 0; j <= num_steps; ++j) {
      up_.push_back(std::pow(1 + u, j));
      down_.push_back(std::pow(1 + d, j));
    }
  }
  double operator()(int n, int k) const { return s0_ * up_[k] * down_[n - k]; }

 private:
  double s0_;
  std::vector<double> up_;    // (1+u)^j.
  std::vector<double> down_;  // (1+d)^j.
};

// Sections 8.1.2-8.1.3: backward induction, values[n][k] at every node.
// European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r); American:
// the larger of that and the payoff from exercising now.
inline Tree OptionTree(double s0, double u, double d, double r, int num_steps,
                       const Payoff& payoff, bool american = false) {
  const double q = RiskNeutralProb(u, d, r);
  const StockPrices price(s0, u, d, num_steps);
  Tree values(num_steps + 1);
  for (int k = 0; k <= num_steps; ++k) {
    values[num_steps].push_back(payoff(price(num_steps, k)));
  }
  for (int n = num_steps - 1; n >= 0; --n) {
    values[n].reserve(n + 1);
    for (int k = 0; k <= n; ++k) {
      double v =
          (q * values[n + 1][k + 1] + (1 - q) * values[n + 1][k]) / (1 + r);
      if (american) v = std::max(v, payoff(price(n, k)));
      values[n].push_back(v);
    }
  }
  return values;
}

// (x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in
// units of the money market worth (1+r)^n.
inline std::pair<double, double> ReplicatingPosition(double s0, double u,
                                                     double d, double r,
                                                     const Tree& values, int n,
                                                     int k) {
  const double s_up = StockPrice(s0, u, d, n + 1, k + 1);
  const double s_down = StockPrice(s0, u, d, n + 1, k);
  const double x = (values[n + 1][k + 1] - values[n + 1][k]) / (s_up - s_down);
  const double y =
      (values[n][k] - x * StockPrice(s0, u, d, n, k)) / std::pow(1 + r, n);
  return {x, y};
}

// Calls f(k, w_k) for k = 0..N with w_k = C(N,k) p^k (1-p)^(N-k). The weights
// are built in log space, updating log C(N, k) from k to k+1: C(N, k) on its
// own passes DBL_MAX near N = 1030 even when w_k is tiny.
template <typename F>
void ForEachBinomialWeight(double p, int num_steps, F f) {
  if (p <= 0 || p >= 1) {
    for (int k = 0; k <= num_steps; ++k) {
      f(k, k == (p <= 0 ? 0 : num_steps) ? 1.0 : 0.0);
    }
    return;
  }
  const double log_p = std::log(p);
  const double log_q = std::log1p(-p);
  double log_c = 0.0;
  for (int k = 0; k <= num_steps; ++k) {
    f(k, std::exp(log_c + k * log_p + (num_steps - k) * log_q));
    if (k < num_steps) log_c += std::log(num_steps - k) - std::log(k + 1.0);
  }
}

// P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k).
inline double BinomialSum(double p, int num_steps, int m) {
  double total = 0.0;
  ForEachBinomialWeight(p, num_steps, [&](int k, double w) {
    if (k >= m) total += w;
  });
  return total;
}

// Section 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k)).
inline double EuropeanPriceSum(double s0, double u, double d, double r,
                               int num_steps, const Payoff& payoff) {
  double total = 0.0;
  ForEachBinomialWeight(
      RiskNeutralProb(u, d, r), num_steps, [&](int k, double w) {
        total += w * payoff(StockPrice(s0, u, d, num_steps, k));
      });
  return total * std::pow(1 + r, -num_steps);
}

// Section 8.1.4: the Cox-Ross-Rubinstein formula,
// C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*), with m the least number
// of up moves for which S(N) > X and p' = p* (1+u) / (1+r).
struct CrrCallResult {
  double price;
  int m;
};

inline CrrCallResult CrrCall(double s0, double strike, double u, double d,
                             double r, int num_steps) {
  const double q = RiskNeutralProb(u, d, r);
  int m = 0;
  while (m <= num_steps && StockPrice(s0, u, d, num_steps, m) <= strike) ++m;
  const double q_prime = q * (1 + u) / (1 + r);
  return {
      s0 * BinomialSum(q_prime, num_steps, m) -
          strike * std::pow(1 + r, -num_steps) * BinomialSum(q, num_steps, m),
      m};
}

// Section 8.2: American options. Nodes (n, k), n < N, where exercising beats
// waiting.
inline std::vector<std::pair<int, int>> EarlyExerciseNodes(
    double s0, double u, double d, double r, int num_steps,
    const Payoff& payoff) {
  const double q = RiskNeutralProb(u, d, r);
  const Tree values = OptionTree(s0, u, d, r, num_steps, payoff, true);
  const StockPrices price(s0, u, d, num_steps);
  std::vector<std::pair<int, int>> nodes;
  for (int n = 0; n < num_steps; ++n) {
    for (int k = 0; k <= n; ++k) {
      const double wait =
          (q * values[n + 1][k + 1] + (1 - q) * values[n + 1][k]) / (1 + r);
      if (payoff(price(n, k)) > wait) nodes.emplace_back(n, k);
    }
  }
  return nodes;
}

}  // namespace m4f::ch08

#endif  // M4F_CH08_BINOMIAL_PRICING_H_
