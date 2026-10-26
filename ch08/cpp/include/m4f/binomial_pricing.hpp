#pragma once
// Sections 8.1-8.2: European and American options in the binomial tree model.
//
// The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a
// money market growing at r per step, and p* = (r - d) / (u - d). Payoffs are
// functions of the final stock price.
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace m4f::ch08 {

using Payoff = std::function<double(double)>;
using Tree = std::vector<std::vector<double>>;

inline double riskNeutralProb(double u, double d, double r) { return (r - d) / (u - d); }

inline double stockPrice(double S0, double u, double d, int n, int k) {
    return S0 * std::pow(1 + u, k) * std::pow(1 + d, n - k);
}

// 8.1.1 One step
struct Replication {
    double x, y, price;  // x shares, y units of the money market (A(0) = 1)
};

// Replicates the payoff Cu / Cd; price = x S(0) + y.
inline Replication oneStepReplication(double S0, double u, double d, double r, double Cu, double Cd) {
    const double x = (Cu - Cd) / (S0 * (u - d));
    const double y = (Cd - x * S0 * (1 + d)) / (1 + r);
    return {x, y, x * S0 + y};
}

// 8.1.2-8.1.3 Backward induction: V[n][k], option values at every node.
// European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r); American: the
// larger of that and the payoff from exercising now.
inline Tree optionTree(double S0, double u, double d, double r, int N, const Payoff& payoff, bool american = false) {
    const double q = riskNeutralProb(u, d, r);
    Tree V(N + 1);
    for (int k = 0; k <= N; ++k) V[N].push_back(payoff(stockPrice(S0, u, d, N, k)));
    for (int n = N - 1; n >= 0; --n)
        for (int k = 0; k <= n; ++k) {
            double v = (q * V[n + 1][k + 1] + (1 - q) * V[n + 1][k]) / (1 + r);
            if (american) v = std::max(v, payoff(stockPrice(S0, u, d, n, k)));
            V[n].push_back(v);
        }
    return V;
}

// (x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in units
// of the money market worth (1+r)^n.
inline std::pair<double, double> replicatingPosition(double S0, double u, double d, double r, const Tree& V, int n,
                                                     int k) {
    const double Su = stockPrice(S0, u, d, n + 1, k + 1), Sd = stockPrice(S0, u, d, n + 1, k);
    const double x = (V[n + 1][k + 1] - V[n + 1][k]) / (Su - Sd);
    const double y = (V[n][k] - x * stockPrice(S0, u, d, n, k)) / std::pow(1 + r, n);
    return {x, y};
}

// P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k)
inline double binomialSum(double p, int N, int m) {
    double total = 0.0, c = 1.0;
    for (int k = 0; k <= N; ++k) {
        if (k >= m) total += c * std::pow(p, k) * std::pow(1 - p, N - k);
        c = c * (N - k) / (k + 1);
    }
    return total;
}

// 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k))
inline double europeanPriceSum(double S0, double u, double d, double r, int N, const Payoff& payoff) {
    const double q = riskNeutralProb(u, d, r);
    double total = 0.0, c = 1.0;
    for (int k = 0; k <= N; ++k) {
        total += c * std::pow(q, k) * std::pow(1 - q, N - k) * payoff(stockPrice(S0, u, d, N, k));
        c = c * (N - k) / (k + 1);
    }
    return total * std::pow(1 + r, -N);
}

// 8.1.4 Cox-Ross-Rubinstein formula: C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*),
// with m the least number of up moves for which S(N) > X and p' = p* (1+u) / (1+r).
struct CrrCall {
    double price;
    int m;
};

inline CrrCall crrCall(double S0, double X, double u, double d, double r, int N) {
    const double q = riskNeutralProb(u, d, r);
    int m = 0;
    while (m <= N && stockPrice(S0, u, d, N, m) <= X) ++m;
    const double qq = q * (1 + u) / (1 + r);
    return {S0 * binomialSum(qq, N, m) - X * std::pow(1 + r, -N) * binomialSum(q, N, m), m};
}

// 8.2 American options: nodes (n, k), n < N, where exercising beats waiting
inline std::vector<std::pair<int, int>> earlyExerciseNodes(double S0, double u, double d, double r, int N,
                                                           const Payoff& payoff) {
    const double q = riskNeutralProb(u, d, r);
    const Tree V = optionTree(S0, u, d, r, N, payoff, true);
    std::vector<std::pair<int, int>> nodes;
    for (int n = 0; n < N; ++n)
        for (int k = 0; k <= n; ++k) {
            const double wait = (q * V[n + 1][k + 1] + (1 - q) * V[n + 1][k]) / (1 + r);
            if (payoff(stockPrice(S0, u, d, n, k)) > wait) nodes.emplace_back(n, k);
        }
    return nodes;
}

}  // namespace m4f::ch08
