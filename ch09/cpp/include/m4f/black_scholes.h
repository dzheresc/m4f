// Black-Scholes prices and the Greek parameters (Section 9.1.2).
// s stock price, strike X, r continuously compounded rate, sigma volatility,
// maturity T time to expiry. Same NormCdf as Chapter 8 (Hart 1968 / G. West).
#ifndef M4F_CH09_BLACK_SCHOLES_H_
#define M4F_CH09_BLACK_SCHOLES_H_

#include <algorithm>
#include <cmath>
#include <utility>

namespace m4f::ch09 {

// Standard normal CDF to double precision.
inline double NormCdf(double x) {
  const double a = std::abs(x);
  double c;
  if (a > 37.0) {
    c = 0.0;
  } else {
    const double e = std::exp(-a * a / 2);
    double b;
    if (a < 7.07106781186547) {
      b = 3.52624965998911e-02 * a + 0.700383064443688;
      b = b * a + 6.37396220353165;
      b = b * a + 33.912866078383;
      b = b * a + 112.079291497871;
      b = b * a + 221.213596169931;
      b = b * a + 220.206867912376;
      c = e * b;
      b = 8.83883476483184e-02 * a + 1.75566716318264;
      b = b * a + 16.064177579207;
      b = b * a + 86.7807322029461;
      b = b * a + 296.564248779674;
      b = b * a + 637.333633378831;
      b = b * a + 793.826512519948;
      b = b * a + 440.413735824752;
      c = c / b;
    } else {
      b = a + 0.65;
      b = a + 4 / b;
      b = a + 3 / b;
      b = a + 2 / b;
      b = a + 1 / b;
      c = e / b / 2.506628274631;
    }
  }
  return x > 0 ? 1 - c : c;
}

inline double NormPdf(double x) {
  return std::exp(-x * x / 2) / 2.506628274631000502;
}

// Quantile of the standard normal, by bisection on NormCdf (0 < p < 1).
inline double NormInv(double p) {
  double lo = -40.0;
  double hi = 40.0;
  for (int i = 0; i < 200; ++i) {
    const double mid = (lo + hi) / 2;
    if (NormCdf(mid) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return (lo + hi) / 2;
}

inline std::pair<double, double> D1D2(double s, double strike, double r,
                                      double sigma, double maturity) {
  const double d1 =
      (std::log(s / strike) + (r + sigma * sigma / 2) * maturity) /
      (sigma * std::sqrt(maturity));
  return {d1, d1 - sigma * std::sqrt(maturity)};
}

inline double Payoff(double s, double strike, bool is_call) {
  return is_call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0);
}

// Black-Scholes price; the payoff when maturity <= 0.
inline double BsPrice(double s, double strike, double r, double sigma,
                      double maturity, bool is_call) {
  if (maturity <= 0) return Payoff(s, strike, is_call);
  const auto [d1, d2] = D1D2(s, strike, r, sigma, maturity);
  if (is_call) {
    return s * NormCdf(d1) - strike * std::exp(-r * maturity) * NormCdf(d2);
  }
  return strike * std::exp(-r * maturity) * NormCdf(-d2) - s * NormCdf(-d1);
}

struct Greeks {
  double price;
  double delta;  // dV/dS.
  double gamma;  // d2V/dS2.
  double theta;  // dV/dt (per year).
  double vega;   // dV/dsigma.
  double rho;    // dV/dr.
};

enum class Greek { kDelta, kGamma, kTheta, kVega, kRho };

inline double GetGreek(const Greeks& g, Greek which) {
  switch (which) {
    case Greek::kDelta:
      return g.delta;
    case Greek::kGamma:
      return g.gamma;
    case Greek::kTheta:
      return g.theta;
    case Greek::kVega:
      return g.vega;
    case Greek::kRho:
      return g.rho;
  }
  return 0.0;
}

// The Greek parameters of a European option in the Black-Scholes model.
inline Greeks BsGreeks(double s, double strike, double r, double sigma,
                       double maturity, bool is_call) {
  const auto [d1, d2] = D1D2(s, strike, r, sigma, maturity);
  const double n1 = NormPdf(d1);
  const double disc = strike * std::exp(-r * maturity);
  const double gamma = n1 / (s * sigma * std::sqrt(maturity));
  const double vega = s * n1 * std::sqrt(maturity);
  if (is_call) {
    return {
        s * NormCdf(d1) - disc * NormCdf(d2),
        NormCdf(d1),
        gamma,
        -s * n1 * sigma / (2 * std::sqrt(maturity)) - r * disc * NormCdf(d2),
        vega,
        maturity * disc * NormCdf(d2)};
  }
  return {disc * NormCdf(-d2) - s * NormCdf(-d1),
          NormCdf(d1) - 1,
          gamma,
          -s * n1 * sigma / (2 * std::sqrt(maturity)) + r * disc * NormCdf(-d2),
          vega,
          -maturity * disc * NormCdf(-d2)};
}

}  // namespace m4f::ch09

#endif  // M4F_CH09_BLACK_SCHOLES_H_
