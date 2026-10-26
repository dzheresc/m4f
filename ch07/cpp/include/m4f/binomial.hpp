#pragma once
// Option prices used to illustrate Sections 7.3-7.5: the Cox-Ross-Rubinstein
// tree with N steps of length tau = T/N, u = e^(sigma sqrt(tau)), d = 1/u,
// growth e^(r tau) per step (Chapter 8 develops the model itself).
#include <algorithm>
#include <cmath>
#include <vector>

namespace m4f::ch07 {

inline double crrPrice(double S0, double X, double r, double sigma, double T, int N, bool isCall,
                       bool american = false) {
    const double tau = T / N;
    const double up = std::exp(sigma * std::sqrt(tau));
    const double down = 1 / up;
    const double growth = std::exp(r * tau);
    const double q = (growth - down) / (up - down);
    const auto payoff = [&](double S) { return isCall ? std::max(S - X, 0.0) : std::max(X - S, 0.0); };

    std::vector<double> values;
    for (int k = 0; k <= N; ++k) values.push_back(payoff(S0 * std::pow(up, k) * std::pow(down, N - k)));
    for (int n = N - 1; n >= 0; --n)
        for (int k = 0; k <= n; ++k) {
            double v = (q * values[k + 1] + (1 - q) * values[k]) / growth;
            if (american) v = std::max(v, payoff(S0 * std::pow(up, k) * std::pow(down, n - k)));
            values[k] = v;
        }
    return values[0];
}

}  // namespace m4f::ch07
