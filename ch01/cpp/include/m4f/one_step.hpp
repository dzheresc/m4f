// Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
// "Introduction: A Simple Market Model" -- the one-step binomial market.
//
// Two assets, two time instants t = 0, 1:
//   risk-free bond  A(0) -> A(1)                     (return r)
//   risky stock     S(0) -> S^u(1) with prob. p      (return u)
//                         S^d(1) with prob. 1 - p    (return d)
// A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.
#pragma once

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

namespace m4f::ch01 {

enum class State { Up, Down };

struct Portfolio {
    double x = 0.0;  // shares of stock
    double y = 0.0;  // units of bond
};

class OneStepMarket {
public:
    OneStepMarket(double A0, double A1, double S0, double Su, double Sd, double p)
        : A0_(A0), A1_(A1), S0_(S0), Su_(Su), Sd_(Sd), p_(p) {
        if (A0 <= 0 || A1 <= 0 || S0 <= 0 || Sd <= 0 || Su <= Sd)
            throw std::invalid_argument("require positive prices and S^u(1) > S^d(1)");
        if (p <= 0 || p >= 1) throw std::invalid_argument("require 0 < p < 1");
    }

    double A0() const { return A0_; }
    double A1() const { return A1_; }
    double S0() const { return S0_; }
    double S1(State s) const { return s == State::Up ? Su_ : Sd_; }
    double p() const { return p_; }

    // Returns K = (P(1) - P(0)) / P(0)
    double r() const { return (A1_ - A0_) / A0_; }
    double u() const { return (Su_ - S0_) / S0_; }
    double d() const { return (Sd_ - S0_) / S0_; }

    // Theorem: no arbitrage  <=>  d < r < u
    bool isArbitrageFree() const { return d() < r() && r() < u(); }

    // Risk-neutral probability p* = (r - d) / (u - d); makes E*[S(1)] = (1+r) S(0)
    double riskNeutralProb() const { return (r() - d()) / (u() - d()); }

    // Portfolio investing `budget` with fraction `w` in stock, the rest in bonds.
    Portfolio split(double budget, double w) const {
        return {w * budget / S0_, (1 - w) * budget / A0_};
    }

    double value0(const Portfolio& P) const { return P.x * S0_ + P.y * A0_; }
    double value1(const Portfolio& P, State s) const { return P.x * S1(s) + P.y * A1_; }
    double portfolioReturn(const Portfolio& P, State s) const {
        return (value1(P, s) - value0(P)) / value0(P);
    }

    // Expectation and standard deviation of a two-point random variable
    // taking values `up` (prob p) and `down` (prob 1-p).
    double expect(double up, double down) const { return p_ * up + (1 - p_) * down; }
    double stddev(double up, double down) const {
        return std::sqrt(p_ * (1 - p_)) * std::abs(up - down);
    }

    // Replicating portfolio for a derivative paying Du (up) / Dd (down) at t=1:
    //   x S^u(1) + y A(1) = Du,   x S^d(1) + y A(1) = Dd
    Portfolio replicate(double Du, double Dd) const {
        const double x = (Du - Dd) / (Su_ - Sd_);
        const double y = (Du - x * Su_) / A1_;
        return {x, y};
    }
    // No-arbitrage price D(0) = value of the replicating portfolio ...
    double price(double Du, double Dd) const { return value0(replicate(Du, Dd)); }
    // ... which equals the discounted risk-neutral expectation.
    double riskNeutralPrice(double Du, double Dd) const {
        const double q = riskNeutralProb();
        return (q * Du + (1 - q) * Dd) / (1 + r());
    }

    // Forward price for delivery at t=1: F = S(0) (1 + r)
    double forwardPrice() const { return S0_ * (1 + r()); }

    // If the model admits arbitrage, a portfolio with V(0)=0, V(1)>=0 and
    // V(1)>0 in some state, using one share; otherwise nullopt.
    std::optional<Portfolio> arbitrage() const {
        if (r() >= u()) return Portfolio{-1.0, S0_ / A0_};  // short stock, buy bonds
        if (r() <= d()) return Portfolio{1.0, -S0_ / A0_};  // borrow, buy stock
        return std::nullopt;
    }

private:
    double A0_, A1_, S0_, Su_, Sd_, p_;
};

inline double callPayoff(double S, double X) { return S > X ? S - X : 0.0; }
inline double putPayoff(double S, double X) { return X > S ? X - S : 0.0; }


// Section 1.7: cost at t=1 of buying one share for an investor who at t=0
// borrowed to buy n calls with strike X: S(1) - n C(1) + n C(0)(1+r).
inline double hedgedPurchaseCost(const OneStepMarket& m, double X, double n, State s) {
    const double C0 = m.price(callPayoff(m.S1(State::Up), X), callPayoff(m.S1(State::Down), X));
    return m.S1(s) - n * callPayoff(m.S1(s), X) + n * C0 * (1 + m.r());
}

}  // namespace m4f::ch01
