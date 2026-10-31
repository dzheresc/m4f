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

// Inverse of NormCdf. Acklam's rational approximation (relative error below
// 1.2e-9) followed by one Halley step against NormCdf itself, which brings it
// to full double precision, about the cost of two NormCdf calls. p outside
// (0, 1) returns -40 or 40.
inline double NormInv(double p) {
  if (!(p > 0)) return -40.0;
  if (!(p < 1)) return 40.0;
  constexpr double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                          -2.759285104469687e+02, 1.383577518672690e+02,
                          -3.066479806614716e+01, 2.506628277459239e+00};
  constexpr double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                          -1.556989798598866e+02, 6.680131188771972e+01,
                          -1.328068155288572e+01};
  constexpr double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                          -2.400758277161838e+00, -2.549732539343734e+00,
                          4.374664141464968e+00,  2.938163982698783e+00};
  constexpr double d[] = {7.784695709041462e-03, 3.224671290700398e-01,
                          2.445134137142996e+00, 3.754408661907416e+00};
  constexpr double kLow = 0.02425;
  double x;
  if (p < kLow || p > 1 - kLow) {
    // Tails: q = sqrt(-2 ln(min(p, 1-p))).
    const double q = std::sqrt(-2 * std::log(p < kLow ? p : 1 - p));
    x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
        ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
    if (p > 1 - kLow) x = -x;
  } else {
    const double q = p - 0.5;
    const double r = q * q;
    x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) *
        q / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
  }
  // Halley step on NormCdf(x) - p.
  const double e = NormCdf(x) - p;
  const double u = e * 2.506628274631000502 * std::exp(x * x / 2);
  return x - u / (1 + x * u / 2);
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
  // Each NormCdf is evaluated once; std::exp may set errno, so the compiler
  // cannot merge repeated calls on its own.
  const double theta0 = -s * n1 * sigma / (2 * std::sqrt(maturity));
  if (is_call) {
    const double nd1 = NormCdf(d1);
    const double nd2 = NormCdf(d2);
    return {s * nd1 - disc * nd2,    nd1,  gamma,
            theta0 - r * disc * nd2, vega, maturity * disc * nd2};
  }
  const double nmd1 = NormCdf(-d1);
  const double nmd2 = NormCdf(-d2);
  return {disc * nmd2 - s * nmd1,
          NormCdf(d1) - 1,
          gamma,
          theta0 + r * disc * nmd2,
          vega,
          -maturity * disc * nmd2};
}

}  // namespace m4f::ch09

#endif  // M4F_CH09_BLACK_SCHOLES_H_
