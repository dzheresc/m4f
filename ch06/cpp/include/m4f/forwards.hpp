#pragma once
// Section 6.1, forward contracts, with continuous compounding at rate r.
// Times are in years; F(t, T) is the forward price at t for delivery at T.
#include <cmath>
#include <utility>
#include <vector>

namespace m4f::ch06 {

// 6.1.1 Forward price
// F(t, T) = S(t) e^(r (T - t)), tau = T - t
inline double forwardPrice(double S, double r, double tau) { return S * std::exp(r * tau); }

// Dividends (amount, time) paid before T: F(0, T) = (S(0) - PV(div)) e^(r T)
inline double forwardPriceDividends(double S0, double r, double T,
                                    const std::vector<std::pair<double, double>>& dividends) {
    double pv = 0.0;
    for (const auto& [amount, t] : dividends) pv += amount * std::exp(-r * t);
    return (S0 - pv) * std::exp(r * T);
}

// Continuous dividend yield rDiv (or a foreign interest rate for currencies):
// F(0, T) = S(0) e^((r - rDiv) T)
inline double forwardPriceYield(double S0, double r, double rDiv, double T) {
    return S0 * std::exp((r - rDiv) * T);
}

// Riskless profit at T per unit from a mispriced forward. If FQuoted is above
// S(0) e^(rT): borrow S(0), buy the stock, sell forward (cash and carry); if
// below: short the stock, invest S(0), buy forward. Either way the profit is
// |FQuoted - S(0) e^(rT)|.
inline double forwardArbitrageProfit(double FQuoted, double S0, double r, double T) {
    return std::abs(FQuoted - forwardPrice(S0, r, T));
}

// 6.1.2 Value at t of a long forward entered at 0: V(t) = (F(t,T) - F(0,T)) e^(-r (T-t))
inline double longForwardValue(double F0, double Ft, double r, double tau) { return (Ft - F0) * std::exp(-r * tau); }

}  // namespace m4f::ch06
