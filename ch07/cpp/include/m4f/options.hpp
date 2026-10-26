#pragma once
// Sections 7.1-7.3: option payoffs, put-call parity and bounds on option prices.
// Continuous compounding at rate r; T is the time to expiry in years and X the
// strike. div is the present value of dividends paid before T.
#include <algorithm>
#include <cmath>

namespace m4f::ch07 {

// 7.1 Definitions
inline double callPayoff(double S, double X) { return std::max(S - X, 0.0); }
inline double putPayoff(double S, double X) { return std::max(X - S, 0.0); }

// Profit at T of a long position bought for `premium` at 0 (premium financed
// at r); a short position earns the negative of it.
inline double profit(double payoff, double premium, double r, double T) { return payoff - premium * std::exp(r * T); }

// 7.2 Put-call parity: C - P = S(0) - div - X e^(-rT).
// Returns C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy the put
// and the stock, borrow; negative: the reverse. Either locks in |gap| e^(rT) at T.
inline double parityGap(double C, double P, double S0, double X, double r, double T, double div = 0.0) {
    return C - P - (S0 - div - X * std::exp(-r * T));
}

struct Bounds {
    double lo, hi;
};

// 7.3.1 European options
// max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div
inline Bounds europeanCallBounds(double S0, double X, double r, double T, double div = 0.0) {
    return {std::max(0.0, S0 - div - X * std::exp(-r * T)), S0 - div};
}

// max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT)
inline Bounds europeanPutBounds(double S0, double X, double r, double T, double div = 0.0) {
    return {std::max(0.0, X * std::exp(-r * T) - S0 + div), X * std::exp(-r * T)};
}

// How far a quoted price lies outside [lo, hi] (0 if inside); this much can be
// locked in at time 0 by trading against the quote.
inline double boundViolation(double price, Bounds b) {
    return price < b.lo ? b.lo - price : (price > b.hi ? price - b.hi : 0.0);
}

// 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends)
inline Bounds americanParityBounds(double S0, double X, double r, double T) {
    return {S0 - X, S0 - X * std::exp(-r * T)};
}

// 7.4.1 Dependence on the strike, for X1 < X2 < X3. Convexity in X:
// C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1). A positive return value is
// the arbitrage profit from writing the middle option and buying the wings.
inline double butterflyGap(double C1, double C2, double C3, double X1, double X2, double X3) {
    const double lam = (X3 - X2) / (X3 - X1);
    return C2 - (lam * C1 + (1 - lam) * C3);
}

}  // namespace m4f::ch07
