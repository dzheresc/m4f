#pragma once
// Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree that
// converges to it (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma sqrt(tau)),
// 1+r = e^(r tau), tau = T/N). r is the continuously compounded rate.
#include "m4f/binomial_pricing.hpp"

#include <cmath>
#include <utility>

namespace m4f::ch08 {

// Standard normal CDF to double precision (Hart 1968, as given by G. West,
// "Better approximations to cumulative normal functions").
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

inline std::pair<double, double> d1d2(double S, double X, double r, double sigma, double T) {
    const double d1 = (std::log(S / X) + (r + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
    return {d1, d1 - sigma * std::sqrt(T)};
}

// C = S N(d1) - X e^(-rT) N(d2)
inline double bsCall(double S, double X, double r, double sigma, double T) {
    const auto [d1, d2] = d1d2(S, X, r, sigma, T);
    return S * normCdf(d1) - X * std::exp(-r * T) * normCdf(d2);
}

// P = X e^(-rT) N(-d2) - S N(-d1)
inline double bsPut(double S, double X, double r, double sigma, double T) {
    const auto [d1, d2] = d1d2(S, X, r, sigma, T);
    return X * std::exp(-r * T) * normCdf(-d2) - S * normCdf(-d1);
}

// Shares in the replicating portfolio: N(d1)
inline double bsCallDelta(double S, double X, double r, double sigma, double T) {
    return normCdf(d1d2(S, X, r, sigma, T).first);
}

inline double crrTreePrice(double S, double X, double r, double sigma, double T, int N, bool isCall,
                           bool american = false) {
    const double tau = T / N;
    const double u = std::exp(sigma * std::sqrt(tau)) - 1;
    const double d = std::exp(-sigma * std::sqrt(tau)) - 1;
    const double rr = std::exp(r * tau) - 1;
    const Payoff payoff = isCall ? Payoff([X](double s) { return std::max(s - X, 0.0); })
                                 : Payoff([X](double s) { return std::max(X - s, 0.0); });
    return optionTree(S, u, d, rr, N, payoff, american)[0][0];
}

}  // namespace m4f::ch08
