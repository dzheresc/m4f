//! Section 1.5: forward contracts.
//!
//! A portfolio (x, y, z) adds z forward contracts (z > 0 long, z < 0 short)
//! with forward price F to x shares and y bonds; entering a forward costs
//! nothing at t = 0.
//!
//! The forward price F is called `delivery_price` below. Other names follow
//! the book, lower-cased: `s0` = S(0), `s1` = S(1), `a0` = A(0), `a1` = A(1),
//! `r` = the risk-free return over the step.

/// A portfolio of shares, bonds and forward contracts.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ForwardPortfolio {
    /// Shares.
    pub x: f64,
    /// Bonds.
    pub y: f64,
    /// Forward contracts.
    pub z: f64,
}

/// Payoff S(1) - F of a long forward.
pub fn long_forward_payoff(s1: f64, delivery_price: f64) -> f64 {
    s1 - delivery_price
}

/// Payoff F - S(1) of a short forward.
pub fn short_forward_payoff(s1: f64, delivery_price: f64) -> f64 {
    delivery_price - s1
}

/// Proposition 1.2: the no-arbitrage forward price
/// F = S(0) A(1) / A(0) = S(0) (1 + r).
pub fn forward_price(s0: f64, a0: f64, a1: f64) -> f64 {
    s0 * a1 / a0
}

/// V(0) = x S(0) + y A(0).
pub fn value0(portfolio: &ForwardPortfolio, s0: f64, a0: f64) -> f64 {
    portfolio.x * s0 + portfolio.y * a0
}

/// V(1) = x S(1) + y A(1) + z (S(1) - F).
pub fn value1(portfolio: &ForwardPortfolio, s1: f64, a1: f64, delivery_price: f64) -> f64 {
    portfolio.x * s1 + portfolio.y * a1 + portfolio.z * (s1 - delivery_price)
}

/// Cash-and-carry arbitrage against a mispriced forward, one contract:
///
/// ```text
/// F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair
/// F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F
/// ```
///
/// V(0) = 0 and V(1) does not depend on S(1). Returns `None` if F is fair.
pub fn forward_arbitrage(
    s0: f64,
    a0: f64,
    a1: f64,
    delivery_price: f64,
) -> Option<ForwardPortfolio> {
    let fair = forward_price(s0, a0, a1);
    if delivery_price > fair {
        return Some(ForwardPortfolio {
            x: 1.0,
            y: -s0 / a0,
            z: -1.0,
        });
    }
    if delivery_price < fair {
        return Some(ForwardPortfolio {
            x: -1.0,
            y: s0 / a0,
            z: 1.0,
        });
    }
    None
}

/// Exercise 1.6: forward on a currency. Holding the foreign currency earns
/// the foreign rate r_f (a negative carrying cost), so
/// F = S(0)(1+r)/(1+r_f) and r_f = S(0)(1+r)/F - 1.
pub fn implied_foreign_rate(s0: f64, delivery_price: f64, r: f64) -> f64 {
    s0 * (1.0 + r) / delivery_price - 1.0
}
