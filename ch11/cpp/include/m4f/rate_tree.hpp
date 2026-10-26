#pragma once
// Sections 11.1-11.2: the binomial tree model of the short rate.
//
// Time steps of length tau; node (n,k) is reached after n steps of which k were
// "up". A Lattice holds one value per node: L[n][k], k = 0..n. The short rate
// r(n,k) applies over [n, n+1], so the one-step bond is B(n,n+1;k) = e^(-r(n,k) tau)
// and the money market account grows by e^(r tau) each step. Risk-neutral
// probabilities p*(n,k) of an up move may vary by node.
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace m4f::ch11 {

using Lattice = std::vector<std::vector<double>>;
using NodeFn = std::function<double(int, int)>;

// Additive tree: r(n,k) = r0 + k up + (n-k) down (down is negative for a fall).
inline Lattice rateLattice(double r0, double up, double down, int N) {
    Lattice L(N + 1);
    for (int n = 0; n <= N; ++n)
        for (int k = 0; k <= n; ++k) L[n].push_back(r0 + k * up + (n - k) * down);
    return L;
}

inline Lattice constantProbs(double p, int N) {
    Lattice L(N);
    for (int n = 0; n < N; ++n) L[n].assign(n + 1, p);
    return L;
}

// B(n,n+1) = e^(-r tau)
inline double oneStepPrice(double r, double tau) { return std::exp(-r * tau); }

// A(0..len(path)) along a path of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k) tau).
inline std::vector<double> moneyMarket(const Lattice& rates, double tau, const std::string& path) {
    std::vector<double> A = {1.0};
    int k = 0;
    for (std::size_t n = 0; n < path.size(); ++n) {
        A.push_back(A.back() * std::exp(rates[n][k] * tau));
        if (path[n] == 'u') ++k;
    }
    return A;
}

// Backward induction from level N = terminal.size()-1 down to 0:
// V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
// with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
// is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise).
inline Lattice discountBack(const Lattice& rates, const Lattice& probs, double tau, const std::vector<double>& terminal,
                            const NodeFn& payment = nullptr, const NodeFn& early = nullptr) {
    const int N = static_cast<int>(terminal.size()) - 1;
    Lattice V(N + 1);
    for (int n = 0; n <= N; ++n) V[n].assign(n + 1, 0.0);
    V[N] = terminal;
    for (int n = N - 1; n >= 0; --n)
        for (int k = 0; k <= n; ++k) {
            const double p = probs[n][k];
            double cont = p * V[n + 1][k + 1] + (1 - p) * V[n + 1][k];
            if (payment) cont += payment(n, k);
            double v = oneStepPrice(rates[n][k], tau) * cont;
            if (early) v = std::max(v, early(n, k));
            V[n][k] = v;
        }
    return V;
}

// 11.2: B(n,N;k) for all nodes, from B(N,N) = 1.
inline Lattice bondLattice(const Lattice& rates, const Lattice& probs, double tau, int N) {
    return discountBack(rates, probs, tau, std::vector<double>(N + 1, 1.0));
}

// B(0,1), ..., B(0,N)
inline std::vector<double> bondPrices(const Lattice& rates, const Lattice& probs, double tau, int N) {
    std::vector<double> out;
    for (int m = 1; m <= N; ++m) out.push_back(bondLattice(rates, probs, tau, m)[0][0]);
    return out;
}

// y(n,N) = -ln B(n,N) / ((N-n) tau)
inline double yieldRate(double B, int n, int N, double tau) { return -std::log(B) / ((N - n) * tau); }

// 11.2.1: the p*(n,k) that prices the bond with lattice B at node (n,k):
// B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k).
inline double impliedProb(const Lattice& B, const Lattice& rates, double tau, int n, int k) {
    return (B[n][k] * std::exp(rates[n][k] * tau) - B[n + 1][k]) / (B[n + 1][k + 1] - B[n + 1][k]);
}

inline Lattice impliedProbs(const Lattice& B, const Lattice& rates, double tau) {
    const int N = static_cast<int>(B.size()) - 1;
    Lattice P(N - 1);
    for (int n = 0; n < N - 1; ++n)
        for (int k = 0; k <= n; ++k) P[n].push_back(impliedProb(B, rates, tau, n, k));
    return P;
}

}  // namespace m4f::ch11
