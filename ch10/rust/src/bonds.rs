//! Section 10.1: maturity-independent yields. All bonds are priced with one
//! continuously compounded yield y, so B(t,T) = e^(-y(T-t)). A `Bond` is a
//! list of cash flows (t_i, C_i); coupons and the face value are flows.

/// One cash flow.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Flow {
    /// Payment time t_i in years.
    pub t: f64,
    /// Amount C_i.
    pub amount: f64,
}

/// A bond as its cash flows.
pub type Bond = Vec<Flow>;

/// A single payment of `face` at `maturity`.
pub fn zero_coupon(face: f64, maturity: f64) -> Bond {
    vec![Flow {
        t: maturity,
        amount: face,
    }]
}

/// `n` coupons of `coupon` at dt, 2dt, ..., n dt; the face is repaid with
/// the last. (C++ defaults `dt` to 1.)
pub fn coupon_bond(face: f64, coupon: f64, n: i32, dt: f64) -> Bond {
    (0..n)
        .map(|i| Flow {
            t: f64::from(i + 1) * dt,
            amount: coupon + if i == n - 1 { face } else { 0.0 },
        })
        .collect()
}

/// P(t) = sum over t_i > t of C_i e^(-y(t_i - t)). (C++ defaults `t` to 0.)
pub fn price(bond: &[Flow], y: f64, t: f64) -> f64 {
    let mut p = 0.0;
    for flow in bond {
        if flow.t > t {
            p += flow.amount * (-y * (flow.t - t)).exp();
        }
    }
    p
}

/// Price and duration of a bond at yield y and time t.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct PriceDuration {
    /// P(t).
    pub price: f64,
    /// D(t).
    pub duration: f64,
}

/// Price and duration from one pass over the cash flows.
pub fn price_and_duration(bond: &[Flow], y: f64, t: f64) -> PriceDuration {
    let mut p = 0.0;
    let mut s = 0.0;
    for flow in bond {
        if flow.t > t {
            let e = (-y * (flow.t - t)).exp();
            p += flow.amount * e;
            s += (flow.t - t) * flow.amount * e;
        }
    }
    PriceDuration {
        price: p,
        duration: s / p,
    }
}

/// D = sum (t_i - t) C_i e^(-y(t_i - t)) / P; dP/dy = -D P. (C++ defaults
/// `t` to 0.)
pub fn duration(bond: &[Flow], y: f64, t: f64) -> f64 {
    price_and_duration(bond, y, t).duration
}

/// Section 10.1.1: value at time t of one bond bought at 0, if the yield is
/// y from then on: coupons received before t are reinvested at y, the rest
/// is priced at y.
pub fn investment_value(bond: &[Flow], y: f64, t: f64) -> f64 {
    let mut v = 0.0;
    for flow in bond {
        v += if flow.t <= t {
            flow.amount * (y * (t - flow.t)).exp()
        } else {
            flow.amount * (-y * (flow.t - t)).exp()
        };
    }
    v
}

/// Annualised log return over [0,t] when buying at yield y0 and the yield
/// jumps to y.
pub fn log_return(bond: &[Flow], y0: f64, y: f64, t: f64) -> f64 {
    (investment_value(bond, y, t) / price(bond, y0, 0.0)).ln() / t
}

/// Section 10.1.3: portfolios of two bonds. The duration of a portfolio is
/// the value-weighted average of durations. Weights w1 + w2 = 1 with
/// w1 D1 + w2 D2 = D_target, where `d1` = D1 and `d2` = D2 are the
/// durations of the two bonds.
pub fn immunising_weights(d1: f64, d2: f64, d_target: f64) -> (f64, f64) {
    let w1 = (d_target - d2) / (d1 - d2);
    (w1, 1.0 - w1)
}

/// Holdings of two bonds plus cash.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Portfolio {
    /// Number of bond 1.
    pub q1: f64,
    /// Number of bond 2.
    pub q2: f64,
    /// Cash.
    pub cash: f64,
}

/// Value at time t and yield y of portfolio `p` holding bonds `b1` and `b2`.
pub fn portfolio_value(p: &Portfolio, b1: &[Flow], b2: &[Flow], y: f64, t: f64) -> f64 {
    p.q1 * price(b1, y, t) + p.q2 * price(b2, y, t) + p.cash
}

/// Invest `v` in bonds 1 and 2, given their prices and durations, so that
/// the duration equals `d_target`. (One of the two C++ `Immunise`
/// overloads.)
pub fn immunise_priced(v: f64, b1: &PriceDuration, b2: &PriceDuration, d_target: f64) -> Portfolio {
    let (w1, w2) = immunising_weights(b1.duration, b2.duration, d_target);
    Portfolio {
        q1: w1 * v / b1.price,
        q2: w2 * v / b2.price,
        cash: 0.0,
    }
}

/// Invest `v` at time t in bonds 1 and 2 so that the duration equals
/// horizon - t.
pub fn immunise(v: f64, b1: &[Flow], b2: &[Flow], y: f64, t: f64, horizon: f64) -> Portfolio {
    immunise_priced(
        v,
        &price_and_duration(b1, y, t),
        &price_and_duration(b2, y, t),
        horizon - t,
    )
}

/// One date of `dynamic_hedge`.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct HedgeStep {
    /// Time in years.
    pub t: f64,
    /// Yield at t.
    pub y: f64,
    /// Portfolio value at t.
    pub value: f64,
    /// Number of bond 1 after rebalancing.
    pub q1: f64,
    /// Number of bond 2 after rebalancing.
    pub q2: f64,
    /// Duration of the rebalanced portfolio.
    pub duration: f64,
}

/// Section 10.1.4: along a path of (t, y) with t[0] = 0, hold the two
/// bonds, collect coupons into cash (accruing at the yield of the
/// interval), and, if `rebalance`, re-immunise at each step so the duration
/// equals the remaining horizon. `v0` is the capital invested at t = 0.
/// (C++ defaults `rebalance` to true.)
pub fn dynamic_hedge(
    v0: f64,
    b1: &[Flow],
    b2: &[Flow],
    horizon: f64,
    path: &[(f64, f64)],
    rebalance: bool,
) -> Vec<HedgeStep> {
    let (t0, y0) = path[0];
    let mut p = immunise(v0, b1, b2, y0, t0, horizon);
    let mut steps = Vec::with_capacity(path.len());
    steps.push(HedgeStep {
        t: t0,
        y: y0,
        value: v0,
        q1: p.q1,
        q2: p.q2,
        duration: horizon - t0,
    });
    for w in path.windows(2) {
        let (t_prev, y_prev) = w[0];
        let (t, y) = w[1];
        let mut cash = p.cash * (y_prev * (t - t_prev)).exp();
        for (q, b) in [(p.q1, b1), (p.q2, b2)] {
            for flow in b {
                if t_prev < flow.t && flow.t <= t {
                    cash += q * flow.amount * (y_prev * (t - flow.t)).exp();
                }
            }
        }
        // Both bonds are priced once per step; the values equal price() and
        // duration() bit for bit.
        let pd1 = price_and_duration(b1, y, t);
        let pd2 = price_and_duration(b2, y, t);
        p = Portfolio {
            q1: p.q1,
            q2: p.q2,
            cash,
        };
        let v = p.q1 * pd1.price + p.q2 * pd2.price + p.cash;
        if rebalance && t < horizon {
            p = immunise_priced(v, &pd1, &pd2, horizon - t);
        }
        let d = if t < horizon {
            (p.q1 * pd1.price * pd1.duration + p.q2 * pd2.price * pd2.duration) / v
        } else {
            0.0
        };
        steps.push(HedgeStep {
            t,
            y,
            value: v,
            q1: p.q1,
            q2: p.q2,
            duration: d,
        });
    }
    steps
}
