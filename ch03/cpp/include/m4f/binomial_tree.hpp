#pragma once
// Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k) after k
// up moves in n steps, with the money market growing at r per step.
#include <cmath>

namespace m4f::ch03 {

// C(n, k) p^k (1-p)^(n-k)
inline double binomialProb(int n, int k, double p) {
    double c = 1.0;
    for (int i = 0; i < k; ++i) c = c * (n - i) / (i + 1);
    return c * std::pow(p, k) * std::pow(1 - p, n - k);
}

struct BinomialTree {
    double S0, u, d, r;

    // S(n) after k up moves out of n
    double price(int n, int k) const { return S0 * std::pow(1 + u, k) * std::pow(1 + d, n - k); }

    // E(K(n)) = p u + (1-p) d, the same for every step
    double expectedReturn(double p) const { return p * u + (1 - p) * d; }

    // E(S(n)) as a sum over the n+1 nodes at step n
    double expectedPrice(int n, double p) const {
        double s = 0.0;
        for (int k = 0; k <= n; ++k) s += price(n, k) * binomialProb(n, k, p);
        return s;
    }

    // 3.2.1 Risk-neutral probability
    bool isArbitrageFree() const { return d < r && r < u; }

    // p* = (r - d) / (u - d), so that E*(K(n)) = r
    double riskNeutralProb() const { return (r - d) / (u - d); }

    // 3.2.2 Martingale property: S~(n) = (1+r)^-n S(n)
    double discountedPrice(int n, int k) const { return price(n, k) * std::pow(1 + r, -n); }

    // E*(S~(n+1) | S(n) at node (n, k)); equals S~(n) under p*
    double conditionalDiscounted(int n, int k) const {
        const double q = riskNeutralProb();
        return q * discountedPrice(n + 1, k + 1) + (1 - q) * discountedPrice(n + 1, k);
    }
};

}  // namespace m4f::ch03
