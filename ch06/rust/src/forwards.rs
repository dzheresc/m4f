//! Section 6.1, forward contracts, with continuous compounding at rate r.
//!
//! Times are in years; F(t, T) is the forward price at t for delivery at T.
//! Names: `s` = S(t) and `s0` = S(0) are spot prices, `f0` = F(0, T) and
//! `ft` = F(t, T) forward prices, and `tau` = T - t the time left to
//! delivery.

/// Section 6.1.1: forward price F(t, T) = S(t) e^(r (T - t)), tau = T - t.
pub fn forward_price(s: f64, r: f64, tau: f64) -> f64 {
    s * (r * tau).exp()
}

/// Dividends (amount, time) paid before T:
/// F(0, T) = (S(0) - PV(div)) e^(r T).
pub fn forward_price_dividends(s0: f64, r: f64, maturity: f64, dividends: &[(f64, f64)]) -> f64 {
    let mut pv = 0.0;
    for &(amount, t) in dividends {
        pv += amount * (-r * t).exp();
    }
    (s0 - pv) * (r * maturity).exp()
}

/// Continuous dividend yield `r_div` (or a foreign interest rate for
/// currencies): F(0, T) = S(0) e^((r - r_div) T).
pub fn forward_price_yield(s0: f64, r: f64, r_div: f64, maturity: f64) -> f64 {
    s0 * ((r - r_div) * maturity).exp()
}

/// Riskless profit at T per unit from a mispriced forward. If `quoted` is
/// above S(0) e^(rT): borrow S(0), buy the stock, sell forward (cash and
/// carry); if below: short the stock, invest S(0), buy forward. Either way
/// the profit is |quoted - S(0) e^(rT)|.
pub fn forward_arbitrage_profit(quoted: f64, s0: f64, r: f64, maturity: f64) -> f64 {
    (quoted - forward_price(s0, r, maturity)).abs()
}

/// Section 6.1.2: value at t of a long forward entered at 0,
/// V(t) = (F(t,T) - F(0,T)) e^(-r (T-t)).
pub fn long_forward_value(f0: f64, ft: f64, r: f64, tau: f64) -> f64 {
    (ft - f0) * (-r * tau).exp()
}
