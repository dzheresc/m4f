// Section 3.3, other models: the trinomial tree and the continuous-time limit
// of the binomial model (log returns m tau +/- sigma sqrt(tau) with
// probability 1/2).
//
// Book notation: S(0) is `s0`, T the horizon (`horizon`), N the number of
// steps (`num_steps`), K a threshold price (`threshold`).
#ifndef M4F_CH03_OTHER_MODELS_H_
#define M4F_CH03_OTHER_MODELS_H_

#include <algorithm>
#include <cmath>
#include <numbers>

namespace m4f::ch03 {

// 3.3.1 Trinomial tree model: returns u > m > d, money market r.
struct RiskNeutralProbs {
  double p_up;
  double p_mid;
  double p_down;
};

struct TrinomialModel {
  double u;  // Up return per step.
  double m;  // Middle return per step.
  double d;  // Down return per step.
  double r;  // Money market return per step.

  // (p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the
  // risk-neutral probability is not unique in this model.
  RiskNeutralProbs GetRiskNeutralProbs(double p_mid) const {
    const double p_up = (r - p_mid * m - (1 - p_mid) * d) / (u - d);
    return {p_up, p_mid, 1 - p_mid - p_up};
  }

  // p_u, p_d > 0 exactly when 0 < p_m < this bound.
  double MaxMiddleProb() const {
    return std::min((r - d) / (m - d), (u - r) / (u - m));
  }
};

// 3.3.2 Continuous-time limit.

// Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8).
inline double NormCdf(double x) {
  if (x < 0) return 1 - NormCdf(-x);
  const double t = 1 / (1 + 0.2316419 * x);
  const double poly =
      t * (0.319381530 +
           t * (-0.356563782 +
                t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))));
  return 1 - std::exp(-x * x / 2) / std::sqrt(2 * std::numbers::pi) * poly;
}

// E(S(T)) with N steps of length tau = T/N.
inline double BinomialExpectedPrice(double s0, double mu, double sigma,
                                    double horizon, int num_steps) {
  const double tau = horizon / num_steps;
  const double a = mu * tau;
  const double b = sigma * std::sqrt(tau);
  return s0 *
         std::pow(0.5 * std::exp(a + b) + 0.5 * std::exp(a - b), num_steps);
}

// Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T).
inline double LognormalExpectedPrice(double s0, double mu, double sigma,
                                     double horizon) {
  return s0 * std::exp((mu + sigma * sigma / 2) * horizon);
}

// P(S(T) > K) in the N-step binomial model.
inline double BinomialProbAbove(double s0, double mu, double sigma,
                                double horizon, int num_steps,
                                double threshold) {
  const double b = sigma * std::sqrt(horizon / num_steps);
  const double log_threshold = std::log(threshold / s0) - mu * horizon;
  const double log_half_n = num_steps * std::log(0.5);
  double log_c = 0.0;
  double total = 0.0;
  for (int k = 0; k <= num_steps; ++k) {
    if ((2 * k - num_steps) * b > log_threshold) {
      total += std::exp(log_c + log_half_n);
    }
    if (k < num_steps) log_c += std::log(num_steps - k) - std::log(k + 1);
  }
  return total;
}

// Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T).
inline double LognormalProbAbove(double s0, double mu, double sigma,
                                 double horizon, double threshold) {
  return NormCdf((std::log(s0 / threshold) + mu * horizon) /
                 (sigma * std::sqrt(horizon)));
}

}  // namespace m4f::ch03

#endif  // M4F_CH03_OTHER_MODELS_H_
