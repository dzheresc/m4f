#pragma once
// Section 6.2, futures: pricing in the binomial model, marking to market, and
// hedging with futures.
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace m4f::ch06 {

using Vec = std::vector<double>;

// 6.2.1 Pricing. In the binomial tree (S0, u, d, r per step) the futures price
// for delivery at N is f(n) = E*(S(N) | S(n)): f(N) = S(N) and backwards
// f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k). Returns f[n][k] after k up
// moves out of n.
inline std::vector<Vec> futuresTree(double S0, double u, double d, double r, int N) {
    const double q = (r - d) / (u - d);
    std::vector<Vec> f(N + 1);
    for (int k = 0; k <= N; ++k) f[N].push_back(S0 * std::pow(1 + u, k) * std::pow(1 + d, N - k));
    for (int n = N - 1; n >= 0; --n)
        for (int k = 0; k <= n; ++k) f[n].push_back(q * f[n + 1][k + 1] + (1 - q) * f[n + 1][k]);
    return f;
}

struct MarginDay {
    double balance;  // after settlement, before any call
    double call;     // margin call that restores the initial margin
};

// Marking to market for one long contract on `size` units: each day the account
// gains size * (f(n) - f(n-1)); when it falls below the maintenance margin, a
// call restores it to the initial margin.
inline std::vector<MarginDay> marginAccount(const Vec& prices, double size, double initial, double maintenance) {
    double balance = initial;
    std::vector<MarginDay> out;
    for (std::size_t n = 1; n < prices.size(); ++n) {
        balance += size * (prices[n] - prices[n - 1]);
        const double call = balance < maintenance ? initial - balance : 0.0;
        out.push_back({balance, call});
        balance += call;
    }
    return out;
}

// 6.2.2 Hedging with futures. Selling h futures per unit of spot exposure turns
// a spot change dS into dS - h df.
// h* = Cov(dS, df) / Var(df)
inline double minVarianceHedgeRatio(const Vec& dS, const Vec& df, const Vec& p) {
    double mS = 0.0, mf = 0.0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        mS += p[i] * dS[i];
        mf += p[i] * df[i];
    }
    double cov = 0.0, var = 0.0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        cov += p[i] * (dS[i] - mS) * (df[i] - mf);
        var += p[i] * (df[i] - mf) * (df[i] - mf);
    }
    return cov / var;
}

// Standard deviation of dS - h df
inline double hedgedStd(double h, const Vec& dS, const Vec& df, const Vec& p) {
    Vec x;
    for (std::size_t i = 0; i < dS.size(); ++i) x.push_back(dS[i] - h * df[i]);
    double m = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) m += p[i] * x[i];
    double v = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) v += p[i] * (x[i] - m) * (x[i] - m);
    return std::sqrt(v);
}

}  // namespace m4f::ch06
