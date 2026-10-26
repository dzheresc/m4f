// Section 1.5: forward contracts. A portfolio (x, y, z) adds z forward
// contracts (z > 0 long, z < 0 short) with forward price F to x shares and
// y bonds; entering a forward costs nothing at t=0.
#pragma once

#include <optional>

namespace m4f::ch01 {

struct ForwardPortfolio {
    double x = 0.0;  // shares
    double y = 0.0;  // bonds
    double z = 0.0;  // forward contracts
};

inline double longForwardPayoff(double S1, double F) { return S1 - F; }
inline double shortForwardPayoff(double S1, double F) { return F - S1; }

// Proposition 1.2: the No-Arbitrage forward price F = S(0) A(1) / A(0) = S(0)(1+r)
inline double forwardPrice(double S0, double A0, double A1) { return S0 * A1 / A0; }

inline double value0(const ForwardPortfolio& P, double S0, double A0) {
    return P.x * S0 + P.y * A0;
}
inline double value1(const ForwardPortfolio& P, double S1, double A1, double F) {
    return P.x * S1 + P.y * A1 + P.z * (S1 - F);
}

// Cash-and-carry arbitrage against a mispriced forward, one contract:
//   F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair
//   F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F
// V(0) = 0 and V(1) does not depend on S(1).
inline std::optional<ForwardPortfolio> forwardArbitrage(double S0, double A0, double A1,
                                                        double F) {
    const double fair = forwardPrice(S0, A0, A1);
    if (F > fair) return ForwardPortfolio{1.0, -S0 / A0, -1.0};
    if (F < fair) return ForwardPortfolio{-1.0, S0 / A0, 1.0};
    return std::nullopt;
}

// Exercise 1.6: forward on a currency. Holding the foreign currency earns the
// foreign rate r_f (a negative carrying cost), so F = S(0)(1+r)/(1+r_f) and
//   r_f = S(0)(1+r)/F - 1.
inline double impliedForeignRate(double S0, double F, double r) { return S0 * (1 + r) / F - 1; }

}  // namespace m4f::ch01
