#pragma once
// Section 11.3: interest rate derivatives on the binomial tree.
//
// Floating payments are the simple one-period rate fixed at (n,k) and paid at
// n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.
#include "m4f/rate_tree.hpp"

#include <algorithm>
#include <vector>

namespace m4f::ch11 {

inline double floatingRate(const Lattice& rates, double tau, int n, int k) { return 1 / oneStepPrice(rates[n][k], tau) - 1; }

// 11.3.1 Options on a zero-coupon bond maturing at N, expiring at nExp <= N.
inline Lattice bondOption(const Lattice& rates, const Lattice& probs, double tau, int N, int nExp, double X, bool isCall,
                          bool american = false) {
    const Lattice B = bondLattice(rates, probs, tau, N);
    const auto payoff = [X, isCall](double b) { return isCall ? std::max(b - X, 0.0) : std::max(X - b, 0.0); };
    std::vector<double> terminal;
    for (int k = 0; k <= nExp; ++k) terminal.push_back(payoff(B[nExp][k]));
    const NodeFn early = american ? NodeFn([&B, payoff](int n, int k) { return payoff(B[n][k]); }) : NodeFn();
    return discountBack(rates, probs, tau, terminal, nullptr, early);
}

// 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on notional 1.
// The fixed rate making the swap worthless: K sum B(0,n) = 1 - B(0,N).
inline double swapRate(const std::vector<double>& B0) {
    double s = 0.0;
    for (double b : B0) s += b;
    return (1 - B0.back()) / s;
}

// Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1.
inline Lattice pricePayments(const Lattice& rates, const Lattice& probs, double tau, int N, const NodeFn& payment) {
    return discountBack(rates, probs, tau, std::vector<double>(N + 1, 0.0), payment);
}

// Receive floating, pay fixed K.
inline Lattice payerSwap(const Lattice& rates, const Lattice& probs, double tau, int N, double K) {
    return pricePayments(rates, probs, tau, N, [&rates, tau, K](int n, int k) { return floatingRate(rates, tau, n, k) - K; });
}

// 11.3.3 Caps and floors: caplet pays max(L(n,k) - K, 0), floorlet max(K - L(n,k), 0).
inline Lattice cap(const Lattice& rates, const Lattice& probs, double tau, int N, double K) {
    return pricePayments(rates, probs, tau, N,
                         [&rates, tau, K](int n, int k) { return std::max(floatingRate(rates, tau, n, k) - K, 0.0); });
}

inline Lattice floor(const Lattice& rates, const Lattice& probs, double tau, int N, double K) {
    return pricePayments(rates, probs, tau, N,
                         [&rates, tau, K](int n, int k) { return std::max(K - floatingRate(rates, tau, n, k), 0.0); });
}

// Value at 0 of the single caplet fixed at nFix and paid at nFix + 1.
inline double caplet(const Lattice& rates, const Lattice& probs, double tau, int nFix, double K) {
    return pricePayments(rates, probs, tau, nFix + 1, [&rates, tau, K, nFix](int n, int k) {
        return n == nFix ? std::max(floatingRate(rates, tau, n, k) - K, 0.0) : 0.0;
    })[0][0];
}

}  // namespace m4f::ch11
