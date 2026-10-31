//! Sections 7.1-7.3: option payoffs, put-call parity and bounds on option
//! prices.
//!
//! Continuous compounding at rate r; `maturity` is the time T to expiry in
//! years and `strike` the strike X. `div` is the present value of dividends
//! paid before T. `s0` is the stock price S(0) now and `s` the stock price
//! S(T) at expiry.

use crate::max;

/// Section 7.1: call payoff (S(T) - X)^+.
pub fn call_payoff(s: f64, strike: f64) -> f64 {
    max(s - strike, 0.0)
}

/// Put payoff (X - S(T))^+.
pub fn put_payoff(s: f64, strike: f64) -> f64 {
    max(strike - s, 0.0)
}

/// Profit at T of a long position bought for `premium` at 0 (premium
/// financed at r); a short position earns the negative of it.
pub fn profit(payoff: f64, premium: f64, r: f64, maturity: f64) -> f64 {
    payoff - premium * (r * maturity).exp()
}

/// Section 7.2: put-call parity, C - P = S(0) - div - X e^(-rT).
///
/// Returns C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy
/// the put and the stock, borrow; negative: the reverse. Either locks in
/// |gap| e^(rT) at T. (C++ defaults `div` to 0.)
pub fn parity_gap(
    call: f64,
    put: f64,
    s0: f64,
    strike: f64,
    r: f64,
    maturity: f64,
    div: f64,
) -> f64 {
    call - put - (s0 - div - strike * (-r * maturity).exp())
}

/// A closed interval [lo, hi] of prices.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Bounds {
    /// Lower bound.
    pub lo: f64,
    /// Upper bound.
    pub hi: f64,
}

/// Section 7.3.1: European calls,
/// max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div.
pub fn european_call_bounds(s0: f64, strike: f64, r: f64, maturity: f64, div: f64) -> Bounds {
    Bounds {
        lo: max(0.0, s0 - div - strike * (-r * maturity).exp()),
        hi: s0 - div,
    }
}

/// European puts: max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT).
pub fn european_put_bounds(s0: f64, strike: f64, r: f64, maturity: f64, div: f64) -> Bounds {
    let discounted = strike * (-r * maturity).exp();
    Bounds {
        lo: max(0.0, discounted - s0 + div),
        hi: discounted,
    }
}

/// How far a quoted price lies outside [lo, hi] (0 if inside); this much
/// can be locked in at time 0 by trading against the quote.
pub fn bound_violation(price: f64, b: Bounds) -> f64 {
    if price < b.lo {
        b.lo - price
    } else if price > b.hi {
        price - b.hi
    } else {
        0.0
    }
}

/// Section 7.3.3: American options,
/// S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends).
pub fn american_parity_bounds(s0: f64, strike: f64, r: f64, maturity: f64) -> Bounds {
    Bounds {
        lo: s0 - strike,
        hi: s0 - strike * (-r * maturity).exp(),
    }
}

/// Section 7.4.1: dependence on the strike, for X1 < X2 < X3. Convexity in
/// X: C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1). A positive
/// return value is the arbitrage profit from writing the middle option and
/// buying the wings. `c1`, `c2`, `c3` are the call prices at strikes `x1`,
/// `x2`, `x3`.
pub fn butterfly_gap(c1: f64, c2: f64, c3: f64, x1: f64, x2: f64, x3: f64) -> f64 {
    let lam = (x3 - x2) / (x3 - x1);
    c2 - (lam * c1 + (1.0 - lam) * c3)
}
