#pragma once
// Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the Fundamental
// Theorem of Asset Pricing.
//
// A one-step market has one stock whose return over the step is one of the
// scenario returns K_i, and a money market returning r. It admits no arbitrage
// exactly when a risk-neutral probability exists: q_i > 0 with sum q_i K_i = r,
// which happens exactly when min K_i < r < max K_i.
#include "m4f/strategies.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <utility>

namespace m4f::ch04 {

inline bool isArbitrageFree(const Vec& returns, double r) {
    return *std::min_element(returns.begin(), returns.end()) < r &&
           r < *std::max_element(returns.begin(), returns.end());
}

// (x, y) with V(0) = x S0 + y A0 = 0 and V(1) >= 0 in every scenario, > 0 in at
// least one; nullopt when the market is arbitrage-free.
inline std::optional<std::pair<double, double>> arbitrageStrategy(const Vec& returns, double r, double S0,
                                                                  double A0) {
    if (r >= *std::max_element(returns.begin(), returns.end())) return std::pair{-1.0, S0 / A0};
    if (r <= *std::min_element(returns.begin(), returns.end())) return std::pair{1.0, -S0 / A0};
    return std::nullopt;
}

// V(1) = x S(1) + y A(1) in each scenario
inline Vec oneStepValues(double x, double y, const Vec& returns, double r, double S0, double A0) {
    Vec v;
    for (double K : returns) v.push_back(x * S0 * (1 + K) + y * A0 * (1 + r));
    return v;
}

// A strictly positive q with sum q_i K_i = r, or nullopt when there is none.
// With two scenarios it is the unique p*; with more, one of many: a mix of the
// uniform distribution (weight w) and a two-point law on the lowest and highest
// return.
inline std::optional<Vec> riskNeutralProbs(const Vec& returns, double r) {
    if (!isArbitrageFree(returns, r)) return std::nullopt;
    const std::size_t n = returns.size();
    const auto ih = std::max_element(returns.begin(), returns.end()) - returns.begin();
    const auto il = std::min_element(returns.begin(), returns.end()) - returns.begin();
    const double hi = returns[ih], lo = returns[il];
    double sum = 0.0;
    for (double K : returns) sum += K;
    const double mean = sum / n;
    double w = 1.0;
    if (mean > lo) w = std::min(w, (r - lo) / (mean - lo));
    if (hi > mean) w = std::min(w, (hi - r) / (hi - mean));
    w = n > 2 ? 0.5 * w : 0.0;
    const double target = (r - w * mean) / (1 - w);
    const double p = (target - lo) / (hi - lo);
    Vec q(n, w / n);
    q[ih] += (1 - w) * p;
    q[il] += (1 - w) * (1 - p);
    return q;
}

// 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares held
// over (n-1, n] from the moves made so far (1 = up, 0 = down; moves.size() = n-1).
using Rule = std::function<double(int, const std::vector<int>&)>;

// E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N and A(0) = 1.
// Under p = p* this equals V(0) for every self-financing rule.
inline double expectedDiscountedValue(double S0, double u, double d, double r, int N, double V0,
                                      const Rule& rule, double p) {
    Vec A;
    for (int n = 0; n <= N; ++n) A.push_back(std::pow(1 + r, n));
    double total = 0.0;
    for (int mask = 0; mask < (1 << N); ++mask) {
        std::vector<int> moves;
        for (int i = 0; i < N; ++i) moves.push_back((mask >> i) & 1);
        Vec S{S0};
        for (int m : moves) S.push_back(S.back() * (1 + (m ? u : d)));
        Vec x;
        for (int n = 1; n <= N; ++n) x.push_back(rule(n, std::vector<int>(moves.begin(), moves.begin() + (n - 1))));
        const auto V = selfFinancing(V0, S, A, x).V;
        int ups = 0;
        for (int m : moves) ups += m;
        total += std::pow(p, ups) * std::pow(1 - p, N - ups) * V[N] / A[N];
    }
    return total;
}

}  // namespace m4f::ch04
