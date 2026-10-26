#pragma once
// Black-Scholes prices and the Greek parameters (Section 9.1.2).
// S stock price, X strike, r continuously compounded rate, sigma volatility,
// T time to expiry. Same normCdf as Chapter 8 (Hart 1968 / G. West).
#include <algorithm>
#include <cmath>
#include <utility>

namespace m4f::ch09 {

inline double normCdf(double x) {
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

inline double normPdf(double x) { return std::exp(-x * x / 2) / 2.506628274631000502; }

// Quantile of the standard normal, by bisection on normCdf (0 < p < 1).
inline double normInv(double p) {
    double lo = -40.0, hi = 40.0;
    for (int i = 0; i < 200; ++i) {
        const double mid = (lo + hi) / 2;
        if (normCdf(mid) < p) lo = mid; else hi = mid;
    }
    return (lo + hi) / 2;
}

inline std::pair<double, double> d1d2(double S, double X, double r, double sigma, double T) {
    const double d1 = (std::log(S / X) + (r + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
    return {d1, d1 - sigma * std::sqrt(T)};
}

inline double payoff(double S, double X, bool isCall) { return isCall ? std::max(S - X, 0.0) : std::max(X - S, 0.0); }

// Black-Scholes price; the payoff when T <= 0.
inline double bsPrice(double S, double X, double r, double sigma, double T, bool isCall) {
    if (T <= 0) return payoff(S, X, isCall);
    const auto [d1, d2] = d1d2(S, X, r, sigma, T);
    if (isCall) return S * normCdf(d1) - X * std::exp(-r * T) * normCdf(d2);
    return X * std::exp(-r * T) * normCdf(-d2) - S * normCdf(-d1);
}

struct Greeks {
    double price;
    double delta;  // dV/dS
    double gamma;  // d2V/dS2
    double theta;  // dV/dt (per year)
    double vega;   // dV/dsigma
    double rho;    // dV/dr
};

enum class Greek { Delta, Gamma, Theta, Vega, Rho };

inline double get(const Greeks& g, Greek which) {
    switch (which) {
        case Greek::Delta: return g.delta;
        case Greek::Gamma: return g.gamma;
        case Greek::Theta: return g.theta;
        case Greek::Vega: return g.vega;
        case Greek::Rho: return g.rho;
    }
    return 0.0;
}

// The Greek parameters of a European option in the Black-Scholes model.
inline Greeks bsGreeks(double S, double X, double r, double sigma, double T, bool isCall) {
    const auto [d1, d2] = d1d2(S, X, r, sigma, T);
    const double n1 = normPdf(d1);
    const double disc = X * std::exp(-r * T);
    const double gamma = n1 / (S * sigma * std::sqrt(T));
    const double vega = S * n1 * std::sqrt(T);
    if (isCall) {
        return {S * normCdf(d1) - disc * normCdf(d2), normCdf(d1), gamma,
                -S * n1 * sigma / (2 * std::sqrt(T)) - r * disc * normCdf(d2), vega, T * disc * normCdf(d2)};
    }
    return {disc * normCdf(-d2) - S * normCdf(-d1), normCdf(d1) - 1, gamma,
            -S * n1 * sigma / (2 * std::sqrt(T)) + r * disc * normCdf(-d2), vega, -T * disc * normCdf(-d2)};
}

}  // namespace m4f::ch09
