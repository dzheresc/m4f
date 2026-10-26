#pragma once
// Sections 5.1-5.2: risk as the standard deviation of return, and portfolios of
// two securities with weights w1 + w2 = 1.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch05 {

using Vec = std::vector<double>;

// 5.1 Risk, from returns K_i in scenarios with probabilities p_i
inline double mean(const Vec& K, const Vec& p) {
    double s = 0.0;
    for (std::size_t i = 0; i < K.size(); ++i) s += K[i] * p[i];
    return s;
}

inline double covariance(const Vec& K1, const Vec& K2, const Vec& p) {
    const double m1 = mean(K1, p), m2 = mean(K2, p);
    double s = 0.0;
    for (std::size_t i = 0; i < K1.size(); ++i) s += p[i] * (K1[i] - m1) * (K2[i] - m2);
    return s;
}

inline double stdDev(const Vec& K, const Vec& p) { return std::sqrt(covariance(K, K, p)); }

inline double correlation(const Vec& K1, const Vec& K2, const Vec& p) {
    return covariance(K1, K2, p) / (stdDev(K1, p) * stdDev(K2, p));
}

// 5.2.1 Two securities: w1 in the first, w2 = 1 - w1 in the second
// mu_V = w1 mu1 + w2 mu2
inline double twoAssetMean(double w1, double mu1, double mu2) { return w1 * mu1 + (1 - w1) * mu2; }

// sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0 against
// rounding when |rho| = 1 makes the portfolio riskless.
inline double twoAssetStd(double w1, double s1, double s2, double rho) {
    const double w2 = 1 - w1;
    return std::sqrt(std::max(0.0, w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2 * w1 * w2 * rho * s1 * s2));
}

// w1 of the minimum variance portfolio: (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2)
inline double minVarianceWeight(double s1, double s2, double rho) {
    return (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2);
}

}  // namespace m4f::ch05
