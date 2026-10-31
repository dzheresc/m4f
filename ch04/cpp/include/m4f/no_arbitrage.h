// Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the Fundamental
// Theorem of Asset Pricing.
//
// A one-step market has one stock whose return over the step is one of the
// scenario returns K_i, and a money market returning r. It admits no arbitrage
// exactly when a risk-neutral probability exists: q_i > 0 with
// sum q_i K_i = r, which happens exactly when min K_i < r < max K_i.
//
// Names follow the book, lower-cased: s0 = S(0) and a0 = A(0) are the stock
// and money market prices at time 0, v0 = V(0) is the initial wealth, u and d
// are the up and down returns of the binomial tree, p is the probability of
// an up move, and x, y are the numbers of shares and money market units held.
#ifndef M4F_CH04_NO_ARBITRAGE_H_
#define M4F_CH04_NO_ARBITRAGE_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

#include "m4f/strategies.h"

namespace m4f::ch04 {

inline bool IsArbitrageFree(const Vec& returns, double r) {
  const auto [lo, hi] = std::minmax_element(returns.begin(), returns.end());
  return *lo < r && r < *hi;
}

// (x, y) with V(0) = x S(0) + y A(0) = 0 and V(1) >= 0 in every scenario,
// > 0 in at least one; nullopt when the market is arbitrage-free.
inline std::optional<std::pair<double, double>> ArbitrageStrategy(
    const Vec& returns, double r, double s0, double a0) {
  if (r >= *std::max_element(returns.begin(), returns.end())) {
    return std::pair{-1.0, s0 / a0};
  }
  if (r <= *std::min_element(returns.begin(), returns.end())) {
    return std::pair{1.0, -s0 / a0};
  }
  return std::nullopt;
}

// V(1) = x S(1) + y A(1) in each scenario.
inline Vec OneStepValues(double x, double y, const Vec& returns, double r,
                         double s0, double a0) {
  Vec values;
  for (double k : returns)
    values.push_back(x * s0 * (1 + k) + y * a0 * (1 + r));
  return values;
}

// A strictly positive q with sum q_i K_i = r, or nullopt when there is none.
// With two scenarios it is the unique p*; with more, one of many: a mix of the
// uniform distribution (weight w) and a two-point law on the lowest and
// highest return.
inline std::optional<Vec> RiskNeutralProbs(const Vec& returns, double r) {
  if (!IsArbitrageFree(returns, r)) return std::nullopt;
  const std::size_t n = returns.size();
  const auto i_hi =
      std::max_element(returns.begin(), returns.end()) - returns.begin();
  const auto i_lo =
      std::min_element(returns.begin(), returns.end()) - returns.begin();
  const double hi = returns[i_hi];
  const double lo = returns[i_lo];
  double sum = 0.0;
  for (double k : returns) sum += k;
  const double mean = sum / n;
  double w = 1.0;
  if (mean > lo) w = std::min(w, (r - lo) / (mean - lo));
  if (hi > mean) w = std::min(w, (hi - r) / (hi - mean));
  w = n > 2 ? 0.5 * w : 0.0;
  const double target = (r - w * mean) / (1 - w);
  const double p = (target - lo) / (hi - lo);
  Vec q(n, w / n);
  q[i_hi] += (1 - w) * p;
  q[i_lo] += (1 - w) * (1 - p);
  return q;
}

// 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares held
// over (n-1, n] from the moves made so far (1 = up, 0 = down;
// moves.size() = n-1).
using Rule = std::function<double(int, const std::vector<int>&)>;

// E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N and A(0) = 1.
// Under p = p* this equals V(0) for every self-financing rule.
//
// The paths are walked depth first, so paths share their common prefix: the
// rule is called once per tree node (2^N - 1 calls in all, with the moves so
// far), and nothing is allocated per path.
inline double ExpectedDiscountedValue(double s0, double u, double d, double r,
                                      int num_steps, double v0,
                                      const Rule& rule, double p) {
  Vec a;
  a.reserve(num_steps + 1);
  for (int n = 0; n <= num_steps; ++n) a.push_back(std::pow(1 + r, n));
  std::vector<int> moves;
  moves.reserve(num_steps);
  double total = 0.0;
  // At time n-1 with stock price s and strategy value v, reached with
  // probability `weight`; chooses x(n) and branches on the n-th move.
  const auto walk = [&](const auto& self, int n, double s, double v,
                        double weight) -> void {
    if (n > num_steps) {
      total += weight * v / a[num_steps];
      return;
    }
    const double x = rule(n, moves);
    const double y = (v - x * s) / a[n - 1];
    for (int move : {0, 1}) {
      const double s_next = s * (1 + (move ? u : d));
      moves.push_back(move);
      self(self, n + 1, s_next, y * a[n] + x * s_next,
           weight * (move ? p : 1 - p));
      moves.pop_back();
    }
  };
  walk(walk, 1, s0, v0, 1.0);
  return total;
}

}  // namespace m4f::ch04

#endif  // M4F_CH04_NO_ARBITRAGE_H_
