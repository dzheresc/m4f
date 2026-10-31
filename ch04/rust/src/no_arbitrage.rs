//! Sections 4.1.2-4.1.4: no arbitrage, the binomial tree and the
//! Fundamental Theorem of Asset Pricing.
//!
//! A one-step market has one stock whose return over the step is one of the
//! scenario returns K_i, and a money market returning r. It admits no
//! arbitrage exactly when a risk-neutral probability exists: q_i > 0 with
//! sum q_i K_i = r, which happens exactly when min K_i < r < max K_i.
//!
//! Names follow the book, lower-cased: `s0` = S(0) and `a0` = A(0) are the
//! stock and money market prices at time 0, `v0` = V(0) is the initial
//! wealth, `u` and `d` are the up and down returns of the binomial tree, `p`
//! is the probability of an up move, and `x`, `y` are the numbers of shares
//! and money market units held.

/// Index of the first smallest entry (like `std::min_element`). Keeps the
/// smallest value in a register instead of reloading `values[lo]`.
fn first_min(values: &[f64]) -> usize {
    let mut lo = 0;
    let mut lo_value = values[0];
    for (i, &v) in values.iter().enumerate().skip(1) {
        if v < lo_value {
            lo = i;
            lo_value = v;
        }
    }
    lo
}

/// Index of the first largest entry (like `std::max_element`).
fn first_max(values: &[f64]) -> usize {
    let mut hi = 0;
    let mut hi_value = values[0];
    for (i, &v) in values.iter().enumerate().skip(1) {
        if v > hi_value {
            hi = i;
            hi_value = v;
        }
    }
    hi
}

/// The smallest and largest entries (like `std::minmax_element`, by value).
fn min_max(values: &[f64]) -> (f64, f64) {
    let (mut lo, mut hi) = (values[0], values[0]);
    for &v in &values[1..] {
        if v < lo {
            lo = v;
        }
        if v > hi {
            hi = v;
        }
    }
    (lo, hi)
}

/// No arbitrage <=> min K_i < r < max K_i.
pub fn is_arbitrage_free(returns: &[f64], r: f64) -> bool {
    let (lo, hi) = min_max(returns);
    lo < r && r < hi
}

/// (x, y) with V(0) = x S(0) + y A(0) = 0 and V(1) >= 0 in every scenario,
/// > 0 in at least one; `None` when the market is arbitrage-free.
pub fn arbitrage_strategy(returns: &[f64], r: f64, s0: f64, a0: f64) -> Option<(f64, f64)> {
    if r >= returns[first_max(returns)] {
        return Some((-1.0, s0 / a0));
    }
    if r <= returns[first_min(returns)] {
        return Some((1.0, -s0 / a0));
    }
    None
}

/// V(1) = x S(1) + y A(1) in each scenario.
pub fn one_step_values(x: f64, y: f64, returns: &[f64], r: f64, s0: f64, a0: f64) -> Vec<f64> {
    returns
        .iter()
        .map(|k| x * s0 * (1.0 + k) + y * a0 * (1.0 + r))
        .collect()
}

/// A strictly positive q with sum q_i K_i = r, or `None` when there is
/// none. With two scenarios it is the unique p*; with more, one of many: a
/// mix of the uniform distribution (weight w) and a two-point law on the
/// lowest and highest return.
pub fn risk_neutral_probs(returns: &[f64], r: f64) -> Option<Vec<f64>> {
    if !is_arbitrage_free(returns, r) {
        return None;
    }
    let n = returns.len();
    let i_hi = first_max(returns);
    let i_lo = first_min(returns);
    let hi = returns[i_hi];
    let lo = returns[i_lo];
    let mut sum = 0.0;
    for k in returns {
        sum += k;
    }
    let mean = sum / n as f64;
    let mut w: f64 = 1.0;
    if mean > lo {
        w = w.min((r - lo) / (mean - lo));
    }
    if hi > mean {
        w = w.min((hi - r) / (hi - mean));
    }
    w = if n > 2 { 0.5 * w } else { 0.0 };
    let target = (r - w * mean) / (1.0 - w);
    let p = (target - lo) / (hi - lo);
    let mut q = vec![w / n as f64; n];
    q[i_hi] += (1.0 - w) * p;
    q[i_lo] += (1.0 - w) * (1.0 - p);
    Some(q)
}

/// 4.1.3 Binomial tree: a strategy is a rule x(n, moves) giving the shares
/// held over (n-1, n] from the moves made so far (1 = up, 0 = down;
/// `moves.len()` = n-1). A trait object, like the C++ `std::function`.
pub type Rule<'a> = &'a dyn Fn(i32, &[i32]) -> f64;

/// E(V~(N)) over all 2^N paths, with V~(N) = V(N) / (1+r)^N and A(0) = 1.
/// Under p = p* this equals V(0) for every self-financing rule.
///
/// The paths are walked depth first, so paths share their common prefix:
/// the rule is called once per tree node (2^N - 1 calls in all, with the
/// moves so far), and nothing is allocated per path.
#[allow(clippy::too_many_arguments)]
pub fn expected_discounted_value(
    s0: f64,
    u: f64,
    d: f64,
    r: f64,
    num_steps: i32,
    v0: f64,
    rule: Rule<'_>,
    p: f64,
) -> f64 {
    let a: Vec<f64> = (0..=num_steps)
        .map(|n| (1.0 + r).powf(f64::from(n)))
        .collect();
    let mut walk = Walk {
        u,
        d,
        p,
        num_steps: num_steps as usize,
        a,
        rule,
        moves: Vec::with_capacity(num_steps as usize),
        total: 0.0,
    };
    walk.visit(1, s0, v0, 1.0);
    walk.total
}

/// State of the depth-first walk in `expected_discounted_value`.
struct Walk<'a> {
    u: f64,
    d: f64,
    p: f64,
    num_steps: usize,
    /// A(n) = (1+r)^n.
    a: Vec<f64>,
    rule: Rule<'a>,
    /// Moves so far.
    moves: Vec<i32>,
    total: f64,
}

impl Walk<'_> {
    /// At time n-1 with stock price `s` and strategy value `v`, reached with
    /// probability `weight`; chooses x(n) and branches on the n-th move.
    fn visit(&mut self, n: usize, s: f64, v: f64, weight: f64) {
        if n > self.num_steps {
            self.total += weight * v / self.a[self.num_steps];
            return;
        }
        let x = (self.rule)(n as i32, &self.moves);
        let y = (v - x * s) / self.a[n - 1];
        for up in [false, true] {
            let s_next = s * (1.0 + if up { self.u } else { self.d });
            self.moves.push(i32::from(up));
            let v_next = y * self.a[n] + x * s_next;
            self.visit(
                n + 1,
                s_next,
                v_next,
                weight * if up { self.p } else { 1.0 - self.p },
            );
            self.moves.pop();
        }
    }
}
