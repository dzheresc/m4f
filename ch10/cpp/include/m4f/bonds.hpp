#pragma once
// Section 10.1: maturity-independent yields. All bonds are priced with one
// continuously compounded yield y, so B(t,T) = e^(-y(T-t)). A Bond is a list of
// cash flows (t_i, C_i); coupons and the face value are flows.
#include <cmath>
#include <utility>
#include <vector>

namespace m4f::ch10 {

struct Flow {
    double t;
    double C;
};
using Bond = std::vector<Flow>;

inline Bond zeroCoupon(double face, double T) { return {{T, face}}; }

// n coupons of `coupon` at dt, 2dt, ..., n dt; the face is repaid with the last.
inline Bond couponBond(double face, double coupon, int n, double dt = 1.0) {
    Bond b;
    for (int i = 0; i < n; ++i) b.push_back({(i + 1) * dt, coupon + (i == n - 1 ? face : 0.0)});
    return b;
}

// P(t) = sum over t_i > t of C_i e^(-y(t_i - t))
inline double price(const Bond& bond, double y, double t = 0.0) {
    double p = 0.0;
    for (const Flow& fl : bond)
        if (fl.t > t) p += fl.C * std::exp(-y * (fl.t - t));
    return p;
}

// D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P
inline double duration(const Bond& bond, double y, double t = 0.0) {
    double s = 0.0;
    for (const Flow& fl : bond)
        if (fl.t > t) s += (fl.t - t) * fl.C * std::exp(-y * (fl.t - t));
    return s / price(bond, y, t);
}

// 10.1.1: value at time t of one bond bought at 0, if the yield is y from then
// on: coupons received before t are reinvested at y, the rest is priced at y.
inline double investmentValue(const Bond& bond, double y, double t) {
    double v = 0.0;
    for (const Flow& fl : bond) v += fl.t <= t ? fl.C * std::exp(y * (t - fl.t)) : fl.C * std::exp(-y * (fl.t - t));
    return v;
}

// Annualised log return over [0,t] when buying at yield y0 and the yield jumps to y.
inline double logReturn(const Bond& bond, double y0, double y, double t) {
    return std::log(investmentValue(bond, y, t) / price(bond, y0)) / t;
}

// 10.1.3 Portfolios of two bonds: the duration of a portfolio is the value-weighted
// average of durations. Weights w1 + w2 = 1 with w1 D1 + w2 D2 = D_target.
inline std::pair<double, double> immunisingWeights(double D1, double D2, double Dtarget) {
    const double w1 = (Dtarget - D2) / (D1 - D2);
    return {w1, 1 - w1};
}

struct Portfolio {
    double q1;  // number of bond 1
    double q2;  // number of bond 2
    double cash;
};

inline double value(const Portfolio& p, const Bond& b1, const Bond& b2, double y, double t) {
    return p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash;
}

// Invest V at time t in bonds 1 and 2 so that the duration equals horizon - t.
inline Portfolio immunise(double V, const Bond& b1, const Bond& b2, double y, double t, double horizon) {
    const auto [w1, w2] = immunisingWeights(duration(b1, y, t), duration(b2, y, t), horizon - t);
    return {w1 * V / price(b1, y, t), w2 * V / price(b2, y, t), 0.0};
}

struct HedgeStep {
    double t;
    double y;
    double value;
    double q1;
    double q2;
    double duration;  // of the rebalanced portfolio
};

// 10.1.4: along a path of (t, y) with t[0] = 0, hold the two bonds, collect
// coupons into cash (accruing at the yield of the interval), and, if `rebalance`,
// re-immunise at each step so the duration equals the remaining horizon.
inline std::vector<HedgeStep> dynamicHedge(double V0, const Bond& b1, const Bond& b2, double horizon,
                                           const std::vector<std::pair<double, double>>& path, bool rebalance = true) {
    const auto [t0, y0] = path[0];
    Portfolio p = immunise(V0, b1, b2, y0, t0, horizon);
    std::vector<HedgeStep> steps = {{t0, y0, V0, p.q1, p.q2, horizon - t0}};
    for (std::size_t i = 1; i < path.size(); ++i) {
        const auto [tPrev, yPrev] = path[i - 1];
        const auto [t, y] = path[i];
        double cash = p.cash * std::exp(yPrev * (t - tPrev));
        for (const auto& [q, b] : {std::pair<double, const Bond*>{p.q1, &b1}, std::pair<double, const Bond*>{p.q2, &b2}})
            for (const Flow& fl : *b)
                if (tPrev < fl.t && fl.t <= t) cash += q * fl.C * std::exp(yPrev * (t - fl.t));
        p = {p.q1, p.q2, cash};
        const double V = value(p, b1, b2, y, t);
        if (rebalance && t < horizon) p = immunise(V, b1, b2, y, t, horizon);
        const double D = t < horizon ? (p.q1 * price(b1, y, t) * duration(b1, y, t) + p.q2 * price(b2, y, t) * duration(b2, y, t)) / V
                                     : 0.0;
        steps.push_back({t, y, V, p.q1, p.q2, D});
    }
    return steps;
}

}  // namespace m4f::ch10
