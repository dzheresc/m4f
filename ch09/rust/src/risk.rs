//! Sections 9.2 and 9.3: Value at Risk and speculation under a lognormal
//! stock,
//!
//! ```text
//! S(T) = S(0) exp(m T + sigma sqrt(T) Z),  Z ~ N(0,1),
//! ```
//!
//! so ln S(T)/S(0) is normal with mean m T and variance sigma^2 T. m is the
//! expected logarithmic return; the Black-Scholes r only enters option
//! prices.
//!
//! A `Strategy` is shares + cash + long/short options held to T. Its
//! terminal value V(S(T)) is assumed non-decreasing in S(T) (long stock,
//! long calls, long puts hedging stock, bull spreads), so quantiles of V are
//! V at quantiles of S(T).

use crate::black_scholes::{bs_price, norm_cdf, norm_inv, payoff};

/// The lognormal model of S(T).
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Lognormal {
    /// S(0).
    pub s0: f64,
    /// Expected logarithmic return per year.
    pub m: f64,
    /// Volatility.
    pub sigma: f64,
    /// Horizon T in years.
    pub maturity: f64,
}

/// The p-quantile of S(T).
#[inline]
pub fn quantile(model: &Lognormal, p: f64) -> f64 {
    model.s0 * (model.m * model.maturity + model.sigma * model.maturity.sqrt() * norm_inv(p)).exp()
}

/// P(S(T) <= s).
#[inline]
pub fn cdf(model: &Lognormal, s: f64) -> f64 {
    norm_cdf(
        ((s / model.s0).ln() - model.m * model.maturity) / (model.sigma * model.maturity.sqrt()),
    )
}

/// E S(T).
#[inline]
pub fn expected_stock(model: &Lognormal) -> f64 {
    model.s0 * (model.m * model.maturity + model.sigma * model.sigma * model.maturity / 2.0).exp()
}

/// E max(S(T)-X, 0) = S0 e^((m+sigma^2/2)T) N(delta1) - X N(delta2).
#[inline]
pub fn expected_call_payoff(model: &Lognormal, strike: f64) -> f64 {
    let sq = model.sigma * model.maturity.sqrt();
    let delta2 = ((model.s0 / strike).ln() + model.m * model.maturity) / sq;
    let delta1 = delta2 + sq;
    expected_stock(model) * norm_cdf(delta1) - strike * norm_cdf(delta2)
}

/// E max(X-S(T), 0) = E max(S(T)-X, 0) - E S(T) + X.
#[inline]
pub fn expected_put_payoff(model: &Lognormal, strike: f64) -> f64 {
    expected_call_payoff(model, strike) - expected_stock(model) + strike
}

/// Options of one strike held to T.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct OptionLeg {
    /// Number of options; negative = written.
    pub qty: f64,
    /// Strike X.
    pub strike: f64,
    /// Call (true) or put (false).
    pub is_call: bool,
}

/// Shares + cash + options, held to T.
#[derive(Clone, Debug, PartialEq)]
pub struct Strategy {
    /// Label used in the report.
    pub name: String,
    /// Shares of stock.
    pub shares: f64,
    /// Invested in the money market at r.
    pub cash: f64,
    /// Options.
    pub legs: Vec<OptionLeg>,
}

/// Value at T of the strategy when the stock ends at `s`.
#[inline]
pub fn terminal_value(st: &Strategy, s: f64, r: f64, maturity: f64) -> f64 {
    let mut v = st.shares * s + st.cash * (r * maturity).exp();
    for leg in &st.legs {
        v += leg.qty * payoff(s, leg.strike, leg.is_call);
    }
    v
}

/// E V(T) under the lognormal model.
pub fn expected_value(st: &Strategy, model: &Lognormal, r: f64) -> f64 {
    let mut v = st.shares * expected_stock(model) + st.cash * (r * model.maturity).exp();
    for leg in &st.legs {
        v += leg.qty
            * if leg.is_call {
                expected_call_payoff(model, leg.strike)
            } else {
                expected_put_payoff(model, leg.strike)
            };
    }
    v
}

/// V(0): the cost of setting up the strategy.
#[inline]
pub fn initial_cost(st: &Strategy, s0: f64, r: f64, sigma: f64, maturity: f64) -> f64 {
    let mut c = st.shares * s0 + st.cash;
    for leg in &st.legs {
        c += leg.qty * bs_price(s0, leg.strike, r, sigma, maturity, leg.is_call);
    }
    c
}

/// Section 9.2.1: VaR at confidence level `conf` = initial value minus the
/// (1-conf) quantile of the terminal value (positive = a loss).
pub fn value_at_risk(st: &Strategy, model: &Lognormal, r: f64, sigma: f64, conf: f64) -> f64 {
    let v0 = initial_cost(st, model.s0, r, sigma, model.maturity);
    v0 - terminal_value(st, quantile(model, 1.0 - conf), r, model.maturity)
}

/// E V(T) / V(0) - 1.
pub fn expected_return(st: &Strategy, model: &Lognormal, r: f64, sigma: f64) -> f64 {
    let v0 = initial_cost(st, model.s0, r, sigma, model.maturity);
    expected_value(st, model, r) / v0 - 1.0
}

/// Stock price at T where the strategy neither gains nor loses
/// (bisection).
pub fn breakeven(st: &Strategy, model: &Lognormal, r: f64, sigma: f64) -> f64 {
    let v0 = initial_cost(st, model.s0, r, sigma, model.maturity);
    let mut lo = 0.0;
    let mut hi = 100.0 * model.s0;
    for _ in 0..200 {
        let mid = (lo + hi) / 2.0;
        if terminal_value(st, mid, r, model.maturity) < v0 {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    (lo + hi) / 2.0
}

/// P(V(T) < V(0)).
pub fn prob_loss(st: &Strategy, model: &Lognormal, r: f64, sigma: f64) -> f64 {
    cdf(model, breakeven(st, model, r, sigma))
}

/// Section 9.2: a holding of `shares` shares, hedged by buying `h` puts per
/// share at the given strike. (C++ defaults `h` to 1.)
pub fn put_hedged_holding(name: &str, shares: f64, strike: f64, h: f64) -> Strategy {
    Strategy {
        name: name.to_string(),
        shares,
        cash: 0.0,
        legs: vec![OptionLeg {
            qty: h * shares,
            strike,
            is_call: false,
        }],
    }
}

/// `shares` shares, no options.
pub fn unhedged_holding(shares: f64) -> Strategy {
    Strategy {
        name: "stock".to_string(),
        shares,
        cash: 0.0,
        legs: Vec::new(),
    }
}

/// Section 9.3.1: spend capital `v0` on shares at stock price `s0` = S(0).
pub fn all_in_stock(v0: f64, s0: f64) -> Strategy {
    Strategy {
        name: "stock".to_string(),
        shares: v0 / s0,
        cash: 0.0,
        legs: Vec::new(),
    }
}

/// Spend capital `v0` on calls at the given strike.
pub fn all_in_calls(
    name: &str,
    v0: f64,
    s0: f64,
    strike: f64,
    r: f64,
    sigma: f64,
    maturity: f64,
) -> Strategy {
    let n = v0 / bs_price(s0, strike, r, sigma, maturity, true);
    Strategy {
        name: name.to_string(),
        shares: 0.0,
        cash: 0.0,
        legs: vec![OptionLeg {
            qty: n,
            strike,
            is_call: true,
        }],
    }
}

/// Buys calls at strike `x1` and writes as many at `x2` (x1 < x2), as many
/// as `v0` pays for.
#[allow(clippy::too_many_arguments)]
pub fn bull_spread(
    name: &str,
    v0: f64,
    s0: f64,
    x1: f64,
    x2: f64,
    r: f64,
    sigma: f64,
    maturity: f64,
) -> Strategy {
    let n = v0
        / (bs_price(s0, x1, r, sigma, maturity, true) - bs_price(s0, x2, r, sigma, maturity, true));
    Strategy {
        name: name.to_string(),
        shares: 0.0,
        cash: 0.0,
        legs: vec![
            OptionLeg {
                qty: n,
                strike: x1,
                is_call: true,
            },
            OptionLeg {
                qty: -n,
                strike: x2,
                is_call: true,
            },
        ],
    }
}

/// Holds strategy `a` scaled by `wa` plus strategy `b` scaled by `wb`.
pub fn mix(name: &str, a: &Strategy, wa: f64, b: &Strategy, wb: f64) -> Strategy {
    let scaled = |legs: &[OptionLeg], w: f64| {
        legs.iter()
            .map(move |l| OptionLeg {
                qty: l.qty * w,
                ..*l
            })
            .collect::<Vec<_>>()
    };
    let mut legs = scaled(&a.legs, wa);
    legs.extend(scaled(&b.legs, wb));
    Strategy {
        name: name.to_string(),
        shares: a.shares * wa + b.shares * wb,
        cash: a.cash * wa + b.cash * wb,
        legs,
    }
}
