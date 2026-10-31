//! Section 9.1: hedging option positions with the stock, the money market
//! and other options. A `Position` is a number of options (negative =
//! written) with a given strike, expiry and type; all options are on the
//! same stock, priced by Black-Scholes with a common r and sigma.

use crate::black_scholes::{Greek, bs_greeks, bs_price, get_greek};

/// A holding of one option series.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Position {
    /// Number of options; negative = written.
    pub qty: f64,
    /// Strike X.
    pub strike: f64,
    /// Time to expiry at t = 0, in years.
    pub expiry: f64,
    /// Call (true) or put (false).
    pub is_call: bool,
}

/// Options held, plus shares and money market balance at time 0.
#[derive(Clone, Debug, PartialEq)]
pub struct Hedge {
    /// The options.
    pub options: Vec<Position>,
    /// Shares of stock.
    pub shares: f64,
    /// Money market balance.
    pub money: f64,
}

/// Value at time `t` when the stock is at `s` (money accrues at e^(rt)).
pub fn portfolio_value(h: &Hedge, s: f64, t: f64, r: f64, sigma: f64) -> f64 {
    let mut v = h.shares * s + h.money * (r * t).exp();
    for p in &h.options {
        v += p.qty * bs_price(s, p.strike, r, sigma, p.expiry - t, p.is_call);
    }
    v
}

/// Sum of qty times the Greek `which` over the options.
///
/// Always inlined, so that a constant `which` reaches `bs_greeks` and the
/// unused Greeks are never computed (GCC does this for the C++ on its own).
#[inline(always)]
pub fn total_greek(options: &[Position], s: f64, r: f64, sigma: f64, which: Greek) -> f64 {
    let mut g = 0.0;
    for p in options {
        g += p.qty
            * get_greek(
                &bs_greeks(s, p.strike, r, sigma, p.expiry, p.is_call),
                which,
            );
    }
    g
}

/// Premium received for the options (minus their value).
pub fn premium(options: &[Position], s: f64, r: f64, sigma: f64) -> f64 {
    let mut v = 0.0;
    for p in options {
        v += p.qty * bs_price(s, p.strike, r, sigma, p.expiry, p.is_call);
    }
    -v
}

/// Section 9.1.1: make the position delta neutral with shares; the money
/// market absorbs the premium received and the cost of the shares (value 0
/// at t=0).
pub fn delta_hedge(options: &[Position], s: f64, r: f64, sigma: f64) -> Hedge {
    let shares = -total_greek(options, s, r, sigma, Greek::Delta);
    Hedge {
        options: options.to_vec(),
        shares,
        money: premium(options, s, r, sigma) - shares * s,
    }
}

/// Section 9.1.2: neutralise delta and one more Greek (gamma or vega) with
/// shares and a position in a second option `extra` (its qty is ignored
/// and solved for).
pub fn delta_greek_hedge(
    options: &[Position],
    extra: &Position,
    s: f64,
    r: f64,
    sigma: f64,
    which: Greek,
) -> Hedge {
    let g_extra = get_greek(
        &bs_greeks(s, extra.strike, r, sigma, extra.expiry, extra.is_call),
        which,
    );
    let qty = -total_greek(options, s, r, sigma, which) / g_extra;
    let mut all = Vec::with_capacity(options.len() + 1);
    all.extend_from_slice(options);
    all.push(Position { qty, ..*extra });
    let shares = -total_greek(&all, s, r, sigma, Greek::Delta);
    let money = premium(&all, s, r, sigma) - shares * s;
    Hedge {
        options: all,
        shares,
        money,
    }
}

/// One rebalancing date of `rebalance_path`.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RebalanceStep {
    /// Time.
    pub t: f64,
    /// Stock price.
    pub s: f64,
    /// Shares held after rebalancing.
    pub shares: f64,
    /// Money market balance after rebalancing.
    pub money: f64,
    /// Mark-to-market value of the whole hedged position.
    pub value: f64,
}

/// Dynamic delta hedging: re-hedge at each price on the path (`prices[0]`
/// at t=0, then one price every `dt` years).
pub fn rebalance_path(
    options: &[Position],
    prices: &[f64],
    dt: f64,
    r: f64,
    sigma: f64,
) -> Vec<RebalanceStep> {
    let mut steps = Vec::with_capacity(prices.len());
    let h = delta_hedge(options, prices[0], r, sigma);
    let mut shares = h.shares;
    let mut money = h.money;
    steps.push(RebalanceStep {
        t: 0.0,
        s: prices[0],
        shares,
        money,
        value: portfolio_value(&h, prices[0], 0.0, r, sigma),
    });
    let mut shifted = Vec::with_capacity(options.len());
    for (i, &s) in prices.iter().enumerate().skip(1) {
        let t = i as f64 * dt;
        money *= (r * dt).exp();
        shifted.clear();
        shifted.extend(options.iter().map(|p| Position {
            expiry: p.expiry - t,
            ..*p
        }));
        let new_shares = -total_greek(&shifted, s, r, sigma, Greek::Delta);
        money -= (new_shares - shares) * s;
        shares = new_shares;
        let mut value = shares * s + money;
        for p in options {
            value += p.qty * bs_price(s, p.strike, r, sigma, p.expiry - t, p.is_call);
        }
        steps.push(RebalanceStep {
            t,
            s,
            shares,
            money,
            value,
        });
    }
    steps
}
