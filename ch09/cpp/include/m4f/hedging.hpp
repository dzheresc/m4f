#pragma once
// Section 9.1: hedging option positions with the stock, the money market and
// other options. A Position is a number of options (negative = written) with a
// given strike, expiry and type; all options are on the same stock, priced by
// Black-Scholes with a common r and sigma.
#include "m4f/black_scholes.hpp"

#include <cmath>
#include <vector>

namespace m4f::ch09 {

struct Position {
    double qty;
    double X;
    double T;
    bool isCall;
};

// Options held, plus shares and money market balance at time 0.
struct Hedge {
    std::vector<Position> options;
    double shares;
    double money;
};

// Value at time t when the stock is at S (money accrues at e^(rt)).
inline double portfolioValue(const Hedge& h, double S, double t, double r, double sigma) {
    double v = h.shares * S + h.money * std::exp(r * t);
    for (const Position& p : h.options) v += p.qty * bsPrice(S, p.X, r, sigma, p.T - t, p.isCall);
    return v;
}

inline double totalGreek(const std::vector<Position>& options, double S, double r, double sigma, Greek which) {
    double g = 0.0;
    for (const Position& p : options) g += p.qty * get(bsGreeks(S, p.X, r, sigma, p.T, p.isCall), which);
    return g;
}

inline double premium(const std::vector<Position>& options, double S, double r, double sigma) {
    double v = 0.0;
    for (const Position& p : options) v += p.qty * bsPrice(S, p.X, r, sigma, p.T, p.isCall);
    return -v;
}

// 9.1.1: make the position delta neutral with shares; the money market
// absorbs the premium received and the cost of the shares (value 0 at t=0).
inline Hedge deltaHedge(const std::vector<Position>& options, double S, double r, double sigma) {
    const double shares = -totalGreek(options, S, r, sigma, Greek::Delta);
    return {options, shares, premium(options, S, r, sigma) - shares * S};
}

// 9.1.2: neutralise delta and one more Greek (gamma or vega) with shares and a
// position in a second option `extra` (its qty is ignored and solved for).
inline Hedge deltaGreekHedge(const std::vector<Position>& options, const Position& extra, double S, double r,
                             double sigma, Greek which) {
    const double gExtra = get(bsGreeks(S, extra.X, r, sigma, extra.T, extra.isCall), which);
    const double qty = -totalGreek(options, S, r, sigma, which) / gExtra;
    std::vector<Position> all = options;
    all.push_back({qty, extra.X, extra.T, extra.isCall});
    const double shares = -totalGreek(all, S, r, sigma, Greek::Delta);
    return {all, shares, premium(all, S, r, sigma) - shares * S};
}

struct RebalanceStep {
    double t;
    double S;
    double shares;
    double money;
    double value;  // mark-to-market value of the whole hedged position
};

// Dynamic delta hedging: re-hedge at each price on the path (prices[0] at t=0).
inline std::vector<RebalanceStep> rebalancePath(const std::vector<Position>& options, const std::vector<double>& prices,
                                                double dt, double r, double sigma) {
    std::vector<RebalanceStep> steps;
    const Hedge h = deltaHedge(options, prices[0], r, sigma);
    double shares = h.shares, money = h.money;
    steps.push_back({0.0, prices[0], shares, money, portfolioValue(h, prices[0], 0.0, r, sigma)});
    for (std::size_t i = 1; i < prices.size(); ++i) {
        const double t = static_cast<double>(i) * dt;
        const double S = prices[i];
        money = money * std::exp(r * dt);
        std::vector<Position> shifted;
        for (const Position& p : options) shifted.push_back({p.qty, p.X, p.T - t, p.isCall});
        const double newShares = -totalGreek(shifted, S, r, sigma, Greek::Delta);
        money -= (newShares - shares) * S;
        shares = newShares;
        double value = shares * S + money;
        for (const Position& p : options) value += p.qty * bsPrice(S, p.X, r, sigma, p.T - t, p.isCall);
        steps.push_back({t, S, shares, money, value});
    }
    return steps;
}

}  // namespace m4f::ch09
