// Sections 5.1-5.2: risk as the standard deviation of return, and portfolios of
// two securities with weights w1 + w2 = 1.
#ifndef M4F_CH05_RISK_H_
#define M4F_CH05_RISK_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch05 {

using Vec = std::vector<double>;

// Section 5.1: risk, from returns k_i in scenarios with probabilities p_i.
inline double Mean(const Vec& k, const Vec& p) {
  double s = 0.0;
  for (std::size_t i = 0; i < k.size(); ++i) s += k[i] * p[i];
  return s;
}

inline double Covariance(const Vec& k1, const Vec& k2, const Vec& p) {
  const double m1 = Mean(k1, p);
  const double m2 = Mean(k2, p);
  double s = 0.0;
  for (std::size_t i = 0; i < k1.size(); ++i) {
    s += p[i] * (k1[i] - m1) * (k2[i] - m2);
  }
  return s;
}

inline double StdDev(const Vec& k, const Vec& p) {
  return std::sqrt(Covariance(k, k, p));
}

inline double Correlation(const Vec& k1, const Vec& k2, const Vec& p) {
  // One pass for the three second moments; same arithmetic as
  // Covariance(k1, k2, p) / (StdDev(k1, p) * StdDev(k2, p)).
  const double m1 = Mean(k1, p);
  const double m2 = Mean(k2, p);
  double cov = 0.0;
  double var1 = 0.0;
  double var2 = 0.0;
  for (std::size_t i = 0; i < k1.size(); ++i) {
    cov += p[i] * (k1[i] - m1) * (k2[i] - m2);
    var1 += p[i] * (k1[i] - m1) * (k1[i] - m1);
    var2 += p[i] * (k2[i] - m2) * (k2[i] - m2);
  }
  return cov / (std::sqrt(var1) * std::sqrt(var2));
}

// Section 5.2.1: two securities, w1 in the first and w2 = 1 - w1 in the
// second. mu_V = w1 mu1 + w2 mu2.
inline double TwoAssetMean(double w1, double mu1, double mu2) {
  return w1 * mu1 + (1 - w1) * mu2;
}

// sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0 against
// rounding when |rho| = 1 makes the portfolio riskless.
inline double TwoAssetStd(double w1, double s1, double s2, double rho) {
  const double w2 = 1 - w1;
  return std::sqrt(std::max(0.0, w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 +
                                     2 * w1 * w2 * rho * s1 * s2));
}

// w1 of the minimum variance portfolio:
// (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2).
inline double MinVarianceWeight(double s1, double s2, double rho) {
  return (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2);
}

}  // namespace m4f::ch05

#endif  // M4F_CH05_RISK_H_
