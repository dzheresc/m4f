#pragma once
// Section 3.3, other models: the trinomial tree and the continuous-time limit of
// the binomial model (log returns m tau +/- sigma sqrt(tau) with probability 1/2).
#include <algorithm>
#include <cmath>
#include <numbers>

namespace m4f::ch03 {

// 3.3.1 Trinomial tree model: returns u > m > d, money market r.
struct RiskNeutralProbs {
    double pu, pm, pd;
};

struct TrinomialModel {
    double u, m, d, r;

    // (p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the
    // risk-neutral probability is not unique in this model.
    RiskNeutralProbs riskNeutralProbs(double pm) const {
        const double pu = (r - pm * m - (1 - pm) * d) / (u - d);
        return {pu, pm, 1 - pm - pu};
    }

    // p_u, p_d > 0 exactly when 0 < p_m < this bound
    double maxMiddleProb() const { return std::min((r - d) / (m - d), (u - r) / (u - m)); }
};

// 3.3.2 Continuous-time limit
// Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8)
inline double normCdf(double x) {
    if (x < 0) return 1 - normCdf(-x);
    const double t = 1 / (1 + 0.2316419 * x);
    const double poly =
        t * (0.319381530 + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))));
    return 1 - std::exp(-x * x / 2) / std::sqrt(2 * std::numbers::pi) * poly;
}

// E(S(T)) with N steps of length tau = T/N
inline double binomialExpectedPrice(double S0, double mu, double sigma, double T, int N) {
    const double tau = T / N;
    const double a = mu * tau, b = sigma * std::sqrt(tau);
    return S0 * std::pow(0.5 * std::exp(a + b) + 0.5 * std::exp(a - b), N);
}

// Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T)
inline double lognormalExpectedPrice(double S0, double mu, double sigma, double T) {
    return S0 * std::exp((mu + sigma * sigma / 2) * T);
}

// P(S(T) > K) in the N-step binomial model
inline double binomialProbAbove(double S0, double mu, double sigma, double T, int N, double K) {
    const double b = sigma * std::sqrt(T / N);
    const double threshold = std::log(K / S0) - mu * T;
    const double logHalfN = N * std::log(0.5);
    double logC = 0.0, total = 0.0;
    for (int k = 0; k <= N; ++k) {
        if ((2 * k - N) * b > threshold) total += std::exp(logC + logHalfN);
        if (k < N) logC += std::log(N - k) - std::log(k + 1);
    }
    return total;
}

// Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T)
inline double lognormalProbAbove(double S0, double mu, double sigma, double T, double K) {
    return normCdf((std::log(S0 / K) + mu * T) / (sigma * std::sqrt(T)));
}

}  // namespace m4f::ch03
