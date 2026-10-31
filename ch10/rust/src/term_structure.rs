//! Section 10.2: general term structure. The market gives zero-coupon bond
//! prices B(0,T) for a grid of maturities T_1 < ... < T_n; all rates are
//! continuously compounded. In a deterministic term structure the implied
//! forward rates are the future short rates, and B(t,T) = B(0,T)/B(0,t).

/// y(0,T) = -ln B(0,T) / T.
pub fn yield_rate(bond_price: f64, maturity: f64) -> f64 {
    -bond_price.ln() / maturity
}

/// f(0,T1,T2) = (ln B(0,T1) - ln B(0,T2)) / (T2 - T1), with `b1` =
/// B(0,T1), `t1` = T1, `b2` = B(0,T2), `t2` = T2. B(0,0) = 1 gives y(0,T2).
pub fn forward_rate(b1: f64, t1: f64, b2: f64, t2: f64) -> f64 {
    (b1.ln() - b2.ln()) / (t2 - t1)
}

/// f(0,T_{i-1},T_i) for each i, with T_0 = 0.
pub fn forward_rates(maturities: &[f64], prices: &[f64]) -> Vec<f64> {
    let mut out = Vec::with_capacity(maturities.len());
    let mut b_prev = 1.0;
    let mut t_prev = 0.0;
    for (&t, &b) in maturities.iter().zip(prices) {
        out.push(forward_rate(b_prev, t_prev, b, t));
        b_prev = b;
        t_prev = t;
    }
    out
}

/// B(0,T_n) = exp(-sum_{i<=n} f(0,T_{i-1},T_i)(T_i - T_{i-1})).
pub fn prices_from_forward_rates(maturities: &[f64], f: &[f64]) -> Vec<f64> {
    let mut out = Vec::with_capacity(maturities.len());
    let mut acc = 0.0;
    let mut t_prev = 0.0;
    for (&t, &f_i) in maturities.iter().zip(f) {
        acc += f_i * (t - t_prev);
        out.push((-acc).exp());
        t_prev = t;
    }
    out
}

/// B(t,T) implied by no arbitrage in a deterministic term structure:
/// B(0,T)/B(0,t), with `b_t` = B(0,t) and `b_maturity` = B(0,T).
pub fn implied_price(b_t: f64, b_maturity: f64) -> f64 {
    b_maturity / b_t
}

/// Price of a coupon bond = sum C_i B(0,t_i), with t_i on the grid (matched
/// exactly). The maturities are ascending, so sorted flow times are matched
/// in one merge pass; a flow time earlier than the one before restarts the
/// scan.
pub fn coupon_bond_price(
    flow_times: &[f64],
    flow_amounts: &[f64],
    maturities: &[f64],
    prices: &[f64],
) -> f64 {
    // Cut the paired slices to their partners' lengths so the loop indexes
    // need no bounds checks.
    let flow_amounts = &flow_amounts[..flow_times.len()];
    let prices = &prices[..maturities.len()];
    let mut total = 0.0;
    let mut j = 0;
    for i in 0..flow_times.len() {
        if i > 0 && flow_times[i] < flow_times[i - 1] {
            j = 0;
        }
        while j < maturities.len() && maturities[j] < flow_times[i] {
            j += 1;
        }
        if j < maturities.len() && maturities[j] == flow_times[i] {
            total += flow_amounts[i] * prices[j];
        }
    }
    total
}

/// The single continuous yield y in [-1, 1] with sum C_i e^(-y t_i) = P.
/// Newton's method (the derivative is minus the duration sum, from the same
/// loop) inside a bisection bracket: a step that leaves the bracket is
/// replaced by its midpoint, so it converges wherever bisection would.
pub fn yield_to_maturity(price: f64, flow_times: &[f64], flow_amounts: &[f64]) -> f64 {
    let flow_amounts = &flow_amounts[..flow_times.len()];
    let mut lo = -1.0;
    let mut hi = 1.0;
    let mut total = 0.0;
    let mut weighted = 0.0;
    for (&t, &c) in flow_times.iter().zip(flow_amounts) {
        total += c;
        weighted += c * t;
    }
    // Starting guess: the yield if all flows were paid at their mean time.
    let mut y = if total > 0.0 && price > 0.0 && weighted > 0.0 {
        (total / price).ln() * total / weighted
    } else {
        0.0
    };
    // `lo < y && y < hi` is false for NaN, like the C++ !(y > lo && y < hi).
    let inside = |y: f64, lo: f64, hi: f64| lo < y && y < hi;
    if !inside(y, lo, hi) {
        y = 0.0;
    }
    for _ in 0..200 {
        let mut s = 0.0;
        let mut ds = 0.0; // -d s / dy.
        for (&t, &c) in flow_times.iter().zip(flow_amounts) {
            let term = c * (-y * t).exp();
            s += term;
            ds += t * term;
        }
        if s > price {
            lo = y;
        } else {
            hi = y;
        }
        if ds > 0.0 {
            let step = (s - price) / ds;
            // Converged: y is the root to within rounding, and may sit on a
            // bracket end, so test this before the bracket.
            let scale = if 1.0 < y.abs() { y.abs() } else { 1.0 };
            if step.abs() <= 1e-15 * scale {
                return y + step;
            }
            y += step;
        }
        if !inside(y, lo, hi) {
            y = (lo + hi) / 2.0;
        }
        if hi - lo <= 1e-15 {
            return y;
        }
    }
    y
}

/// Section 10.2.2: A(T_n) from rolling over one-period bonds at the
/// (implied, hence realised) short rates: A(T_n) = prod 1/B(T_{i-1},T_i) =
/// 1/B(0,T_n). The maturities are not needed; the parameter keeps the C++
/// signature.
pub fn money_market_account(_maturities: &[f64], prices: &[f64]) -> Vec<f64> {
    let mut out = Vec::with_capacity(prices.len());
    let mut a = 1.0;
    let mut b_prev = 1.0;
    for &b in prices {
        a /= implied_price(b_prev, b);
        out.push(a);
        b_prev = b;
    }
    out
}
