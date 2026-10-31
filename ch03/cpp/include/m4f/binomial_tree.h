// Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k) after k
// up moves in n steps, with the money market growing at r per step.
#ifndef M4F_CH03_BINOMIAL_TREE_H_
#define M4F_CH03_BINOMIAL_TREE_H_

#include <cmath>

namespace m4f::ch03 {

// C(n, k) p^k (1-p)^(n-k), computed in log space: C(n, k) alone passes
// DBL_MAX near n = 1030 even when the probability itself is tiny.
inline double BinomialProb(int n, int k, double p) {
  if (k < 0 || k > n) return 0.0;
  if (p <= 0) return k == 0 ? 1.0 : 0.0;
  if (p >= 1) return k == n ? 1.0 : 0.0;
  const double log_c =
      std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0);
  return std::exp(log_c + k * std::log(p) + (n - k) * std::log1p(-p));
}

struct BinomialTree {
  double s0;  // S(0).
  double u;   // Up return.
  double d;   // Down return.
  double r;   // Risk-free return per step.

  // S(n) after k up moves out of n.
  double Price(int n, int k) const {
    return s0 * std::pow(1 + u, k) * std::pow(1 + d, n - k);
  }

  // E(K(n)) = p u + (1-p) d, the same for every step.
  double ExpectedReturn(double p) const { return p * u + (1 - p) * d; }

  // E(S(n)) as a sum over the n+1 nodes at step n, O(n). Each term
  // C(n, k) p^k (1-p)^(n-k) S(n, k) is built in log space, updating log C(n, k)
  // from k to k+1, so nothing overflows for large n.
  double ExpectedPrice(int n, double p) const {
    if (p <= 0) return Price(n, 0);
    if (p >= 1) return Price(n, n);
    const double log_up = std::log(p) + std::log1p(u);
    const double log_down = std::log1p(-p) + std::log1p(d);
    double log_c = 0.0;
    double sum = 0.0;
    for (int k = 0; k <= n; ++k) {
      sum += std::exp(log_c + k * log_up + (n - k) * log_down);
      if (k < n) log_c += std::log(n - k) - std::log(k + 1.0);
    }
    return s0 * sum;
  }

  // 3.2.1 Risk-neutral probability.
  bool IsArbitrageFree() const { return d < r && r < u; }

  // p* = (r - d) / (u - d), so that E*(K(n)) = r.
  double RiskNeutralProb() const { return (r - d) / (u - d); }

  // 3.2.2 Martingale property: S~(n) = (1+r)^-n S(n).
  double DiscountedPrice(int n, int k) const {
    return Price(n, k) * std::pow(1 + r, -n);
  }

  // E*(S~(n+1) | S(n) at node (n, k)); equals S~(n) under p*.
  double ConditionalDiscounted(int n, int k) const {
    const double q = RiskNeutralProb();
    return q * DiscountedPrice(n + 1, k + 1) +
           (1 - q) * DiscountedPrice(n + 1, k);
  }
};

}  // namespace m4f::ch03

#endif  // M4F_CH03_BINOMIAL_TREE_H_
