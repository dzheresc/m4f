#pragma once
// Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal stock,
//     S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
// so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
// expected logarithmic return; the Black-Scholes r only enters option prices.
//
// A Strategy is shares + cash + long/short options held to T. Its terminal
// value V(S(T)) is assumed non-decreasing in S(T) (long stock, long calls, long
// puts hedging stock, bull spreads), so quantiles of V are V at quantiles of S(T).
#include "m4f/black_scholes.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace m4f::ch09 {

struct Lognormal {
    double S0;
    double m;
    double sigma;
    double T;

    double quantile(double p) const { return S0 * std::exp(m * T + sigma * std::sqrt(T) * normInv(p)); }
    // P(S(T) <= s)
    double cdf(double s) const { return normCdf((std::log(s / S0) - m * T) / (sigma * std::sqrt(T))); }
    double expectedStock() const { return S0 * std::exp(m * T + sigma * sigma * T / 2); }
    // E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2)
    double expectedCallPayoff(double X) const {
        const double sq = sigma * std::sqrt(T);
        const double delta2 = (std::log(S0 / X) + m * T) / sq;
        const double delta1 = delta2 + sq;
        return expectedStock() * normCdf(delta1) - X * normCdf(delta2);
    }
    // E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X
    double expectedPutPayoff(double X) const { return expectedCallPayoff(X) - expectedStock() + X; }
};

struct OptionLeg {
    double qty;
    double X;
    bool isCall;
};

struct Strategy {
    std::string name;
    double shares;
    double cash;  // invested in the money market at r
    std::vector<OptionLeg> legs;

    double terminalValue(double S, double r, double T) const {
        double v = shares * S + cash * std::exp(r * T);
        for (const OptionLeg& leg : legs) v += leg.qty * payoff(S, leg.X, leg.isCall);
        return v;
    }
    double expectedValue(const Lognormal& model, double r) const {
        double v = shares * model.expectedStock() + cash * std::exp(r * model.T);
        for (const OptionLeg& leg : legs)
            v += leg.qty * (leg.isCall ? model.expectedCallPayoff(leg.X) : model.expectedPutPayoff(leg.X));
        return v;
    }
};

inline double initialCost(const Strategy& st, double S0, double r, double sigma, double T) {
    double c = st.shares * S0 + st.cash;
    for (const OptionLeg& leg : st.legs) c += leg.qty * bsPrice(S0, leg.X, r, sigma, T, leg.isCall);
    return c;
}

// 9.2.1: VaR at confidence level conf = initial value minus the (1-conf)
// quantile of the terminal value (positive = a loss).
inline double valueAtRisk(const Strategy& st, const Lognormal& model, double r, double sigma, double conf) {
    const double V0 = initialCost(st, model.S0, r, sigma, model.T);
    return V0 - st.terminalValue(model.quantile(1 - conf), r, model.T);
}

inline double expectedReturn(const Strategy& st, const Lognormal& model, double r, double sigma) {
    const double V0 = initialCost(st, model.S0, r, sigma, model.T);
    return st.expectedValue(model, r) / V0 - 1;
}

// Stock price at T where the strategy neither gains nor loses (bisection).
inline double breakeven(const Strategy& st, const Lognormal& model, double r, double sigma) {
    const double V0 = initialCost(st, model.S0, r, sigma, model.T);
    double lo = 0.0, hi = 100.0 * model.S0;
    for (int i = 0; i < 200; ++i) {
        const double mid = (lo + hi) / 2;
        if (st.terminalValue(mid, r, model.T) < V0) lo = mid; else hi = mid;
    }
    return (lo + hi) / 2;
}

inline double probLoss(const Strategy& st, const Lognormal& model, double r, double sigma) {
    return model.cdf(breakeven(st, model, r, sigma));
}

// 9.2: a holding of `shares` shares, hedged by buying `h` puts per share at strike X.
inline Strategy putHedgedHolding(const std::string& name, double shares, double X, double h = 1.0) {
    return {name, shares, 0.0, {{h * shares, X, false}}};
}

inline Strategy unhedgedHolding(double shares) { return {"stock", shares, 0.0, {}}; }

// 9.3.1: spend capital V0 on one instrument.
inline Strategy allInStock(double V0, double S0) { return {"stock", V0 / S0, 0.0, {}}; }

inline Strategy allInCalls(const std::string& name, double V0, double S0, double X, double r, double sigma, double T) {
    const double n = V0 / bsPrice(S0, X, r, sigma, T, true);
    return {name, 0.0, 0.0, {{n, X, true}}};
}

inline Strategy bullSpread(const std::string& name, double V0, double S0, double X1, double X2, double r, double sigma,
                           double T) {
    const double n = V0 / (bsPrice(S0, X1, r, sigma, T, true) - bsPrice(S0, X2, r, sigma, T, true));
    return {name, 0.0, 0.0, {{n, X1, true}, {-n, X2, true}}};
}

inline Strategy mix(const std::string& name, const Strategy& a, double wa, const Strategy& b, double wb) {
    std::vector<OptionLeg> legs;
    for (const OptionLeg& l : a.legs) legs.push_back({l.qty * wa, l.X, l.isCall});
    for (const OptionLeg& l : b.legs) legs.push_back({l.qty * wb, l.X, l.isCall});
    return {name, a.shares * wa + b.shares * wb, a.cash * wa + b.cash * wb, legs};
}

}  // namespace m4f::ch09
