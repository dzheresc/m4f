#pragma once
// Section 3.1, dynamics of stock prices: returns, logarithmic returns and their
// expectations over a finite set of scenarios.
#include <cmath>
#include <cstddef>
#include <vector>

namespace m4f::ch03 {

// 3.1.1 Return
// K(1) = (S(1) + div - S(0)) / S(0)
inline double oneStepReturn(double S0, double S1, double dividend = 0.0) {
    return (S1 + dividend - S0) / S0;
}

// k(1) = ln(S(1) / S(0))
inline double logReturn(double S0, double S1) { return std::log(S1 / S0); }

// One-step returns K(n) along a price path S(0), S(1), ...
inline std::vector<double> returns(const std::vector<double>& path) {
    std::vector<double> out;
    for (std::size_t i = 1; i < path.size(); ++i) out.push_back(oneStepReturn(path[i - 1], path[i]));
    return out;
}

inline std::vector<double> logReturns(const std::vector<double>& path) {
    std::vector<double> out;
    for (std::size_t i = 1; i < path.size(); ++i) out.push_back(logReturn(path[i - 1], path[i]));
    return out;
}

// K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n))
inline double compoundReturn(const std::vector<double>& Ks) {
    double g = 1.0;
    for (double K : Ks) g *= 1 + K;
    return g - 1;
}

// 3.1.2 Expected return
inline double expectation(const std::vector<double>& values, const std::vector<double>& probs) {
    double s = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) s += values[i] * probs[i];
    return s;
}

inline double variance(const std::vector<double>& values, const std::vector<double>& probs) {
    const double mu = expectation(values, probs);
    double s = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const double d = values[i] - mu;
        s += probs[i] * d * d;
    }
    return s;
}

}  // namespace m4f::ch03
