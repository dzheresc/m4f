// Section 4.1, stock and money market models: investment strategies.
//
// Prices are given along one path: s[n] (or s[j][n] for several stocks) and
// a[n] for n = 0..N, standing for the book's S(n) and A(n). A strategy holds
// x(n) shares and y(n) units of the money market over the step from n-1 to n,
// chosen at time n-1; vectors are indexed from 0, so x[0] is x(1).
#ifndef M4F_CH04_STRATEGIES_H_
#define M4F_CH04_STRATEGIES_H_

#include <cstddef>
#include <vector>

namespace m4f::ch04 {

// Shorthands for the price, position and value series used throughout.
using Vec = std::vector<double>;
using Mat = std::vector<Vec>;

struct StrategyPath {
  Vec y;       // y[n-1] = y(n).
  Vec values;  // values[n] = V(n).
};

// Several stocks s[j][n], optional dividends (*dividends)[j][n] paid at n to
// the holders over (n-1, n]. The money market position y(n+1) is set so that
// rebalancing at n costs exactly V(n):
//   sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
//   V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).
inline StrategyPath SelfFinancingMulti(double v0, const Mat& s, const Vec& a,
                                       const Mat& x,
                                       const Mat* dividends = nullptr) {
  const std::size_t num_steps = a.size() - 1;
  StrategyPath path{{}, {v0}};
  path.y.reserve(num_steps);
  path.values.reserve(num_steps + 1);
  for (std::size_t n = 1; n <= num_steps; ++n) {
    double stock = 0.0;
    for (std::size_t j = 0; j < s.size(); ++j)
      stock += x[j][n - 1] * s[j][n - 1];
    path.y.push_back((path.values[n - 1] - stock) / a[n - 1]);
    double value = path.y[n - 1] * a[n];
    for (std::size_t j = 0; j < s.size(); ++j) {
      const double dividend = dividends ? (*dividends)[j][n] : 0.0;
      value += x[j][n - 1] * (s[j][n] + dividend);
    }
    path.values.push_back(value);
  }
  return path;
}

// One stock: y(n) and V(n) of the self-financing strategy with initial value
// v0 and stock positions x. Same arithmetic as SelfFinancingMulti with one
// stock and no dividends, without copying s and x into matrices.
inline StrategyPath SelfFinancing(double v0, const Vec& s, const Vec& a,
                                  const Vec& x) {
  const std::size_t num_steps = a.size() - 1;
  StrategyPath path{{}, {v0}};
  path.y.reserve(num_steps);
  path.values.reserve(num_steps + 1);
  for (std::size_t n = 1; n <= num_steps; ++n) {
    const double y = (path.values[n - 1] - x[n - 1] * s[n - 1]) / a[n - 1];
    path.y.push_back(y);
    path.values.push_back(y * a[n] + x[n - 1] * s[n]);
  }
  return path;
}

// V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1)).
inline double Gains(const Vec& s, const Vec& a, const Vec& x, const Vec& y) {
  double gains = 0.0;
  for (std::size_t n = 1; n < a.size(); ++n) {
    gains += x[n - 1] * (s[n] - s[n - 1]) + y[n - 1] * (a[n] - a[n - 1]);
  }
  return gains;
}

// V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n).
inline double DiscountedGains(const Vec& s, const Vec& a, const Vec& x) {
  double gains = 0.0;
  for (std::size_t n = 1; n < a.size(); ++n) {
    gains += x[n - 1] * (s[n] / a[n] - s[n - 1] / a[n - 1]);
  }
  return gains;
}

}  // namespace m4f::ch04

#endif  // M4F_CH04_STRATEGIES_H_
