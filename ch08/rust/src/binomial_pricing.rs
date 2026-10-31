//! Sections 8.1-8.2: European and American options in the binomial tree
//! model.
//!
//! The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps,
//! a money market growing at r per step, and p* = (r - d) / (u - d). Payoffs
//! are functions of the final stock price. `s0` is the stock price S(0) at
//! the root of the tree.

use crate::max;

/// A payoff as a function of the stock price. A trait object, like the C++
/// `std::function`, so that one compiled tree serves every payoff.
pub type Payoff<'a> = &'a dyn Fn(f64) -> f64;

/// Option values by node: `tree[n][k]` after k up moves in n steps.
pub type Tree = Vec<Vec<f64>>;

/// p* = (r - d) / (u - d).
pub fn risk_neutral_prob(u: f64, d: f64, r: f64) -> f64 {
    (r - d) / (u - d)
}

/// S(0) (1+u)^k (1+d)^(n-k).
pub fn stock_price(s0: f64, u: f64, d: f64, n: i32, k: i32) -> f64 {
    s0 * (1.0 + u).powf(f64::from(k)) * (1.0 + d).powf(f64::from(n - k))
}

/// Section 8.1.1: a one-step replicating portfolio.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Replication {
    /// Shares.
    pub x: f64,
    /// Units of the money market (A(0) = 1).
    pub y: f64,
    /// x S(0) + y.
    pub price: f64,
}

/// Replicates the payoff `c_up` / `c_down`; price = x S(0) + y.
pub fn one_step_replication(
    s0: f64,
    u: f64,
    d: f64,
    r: f64,
    c_up: f64,
    c_down: f64,
) -> Replication {
    let x = (c_up - c_down) / (s0 * (u - d));
    let y = (c_down - x * s0 * (1.0 + d)) / (1.0 + r);
    Replication {
        x,
        y,
        price: x * s0 + y,
    }
}

/// S(0) (1+u)^k (1+d)^(n-k) for 0 <= k <= n <= N, read from two tables of
/// powers built once: the same arithmetic as `stock_price` without two
/// `powf` calls per node.
pub struct StockPrices {
    s0: f64,
    /// (1+u)^j.
    up: Vec<f64>,
    /// (1+d)^j.
    down: Vec<f64>,
}

impl StockPrices {
    /// Tables for a tree of `num_steps` steps.
    pub fn new(s0: f64, u: f64, d: f64, num_steps: i32) -> Self {
        Self {
            s0,
            up: (0..num_steps + 1)
                .map(|j| (1.0 + u).powf(f64::from(j)))
                .collect(),
            down: (0..num_steps + 1)
                .map(|j| (1.0 + d).powf(f64::from(j)))
                .collect(),
        }
    }

    /// S(0).
    #[inline]
    pub fn s0(&self) -> f64 {
        self.s0
    }

    /// (1+u)^j and (1+d)^j for j = 0..=n: node (n, k) has price
    /// S(0) up[k] down[n-k]. Both slices have length n+1, so that a loop
    /// over k = 0..=n can index them without bounds checks.
    #[inline]
    pub fn row_factors(&self, n: usize) -> (&[f64], &[f64]) {
        (&self.up[..n + 1], &self.down[..n + 1])
    }

    /// The stock price at node (n, k).
    #[inline]
    pub fn at(&self, n: usize, k: usize) -> f64 {
        self.s0 * self.up[k] * self.down[n - k]
    }
}

/// Sections 8.1.2-8.1.3: backward induction, `values[n][k]` at every node.
/// European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r);
/// American: the larger of that and the payoff from exercising now. (C++
/// defaults `american` to false.)
pub fn option_tree(
    s0: f64,
    u: f64,
    d: f64,
    r: f64,
    num_steps: i32,
    payoff: Payoff,
    american: bool,
) -> Tree {
    let q = risk_neutral_prob(u, d, r);
    let price = StockPrices::new(s0, u, d, num_steps);
    let steps = num_steps as usize;
    let mut values: Tree = vec![Vec::new(); steps + 1];
    values[steps] = (0..steps + 1).map(|k| payoff(price.at(steps, k))).collect();
    for n in (0..steps).rev() {
        let next = &values[n + 1];
        let row = (0..n + 1)
            .map(|k| {
                let v = (q * next[k + 1] + (1.0 - q) * next[k]) / (1.0 + r);
                if american {
                    max(v, payoff(price.at(n, k)))
                } else {
                    v
                }
            })
            .collect();
        values[n] = row;
    }
    values
}

/// (x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in
/// units of the money market worth (1+r)^n.
pub fn replicating_position(
    s0: f64,
    u: f64,
    d: f64,
    r: f64,
    values: &[Vec<f64>],
    n: i32,
    k: i32,
) -> (f64, f64) {
    let (nu, ku) = (n as usize, k as usize);
    let s_up = stock_price(s0, u, d, n + 1, k + 1);
    let s_down = stock_price(s0, u, d, n + 1, k);
    let x = (values[nu + 1][ku + 1] - values[nu + 1][ku]) / (s_up - s_down);
    let y = (values[nu][ku] - x * stock_price(s0, u, d, n, k)) / (1.0 + r).powf(f64::from(n));
    (x, y)
}

/// Calls `f(k, w_k)` for k = 0..N with w_k = C(N,k) p^k (1-p)^(N-k). The
/// weights are built in log space, updating log C(N, k) from k to k+1:
/// C(N, k) on its own passes `f64::MAX` near N = 1030 even when w_k is tiny.
pub fn for_each_binomial_weight(p: f64, num_steps: i32, mut f: impl FnMut(i32, f64)) {
    if p <= 0.0 || p >= 1.0 {
        let hit = if p <= 0.0 { 0 } else { num_steps };
        for k in 0..num_steps + 1 {
            f(k, if k == hit { 1.0 } else { 0.0 });
        }
        return;
    }
    let log_p = p.ln();
    let log_q = (-p).ln_1p();
    let mut log_c = 0.0;
    for k in 0..num_steps + 1 {
        f(
            k,
            (log_c + f64::from(k) * log_p + f64::from(num_steps - k) * log_q).exp(),
        );
        if k < num_steps {
            log_c += f64::from(num_steps - k).ln() - (f64::from(k) + 1.0).ln();
        }
    }
}

/// P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k).
pub fn binomial_sum(p: f64, num_steps: i32, m: i32) -> f64 {
    let mut total = 0.0;
    for_each_binomial_weight(p, num_steps, |k, w| {
        if k >= m {
            total += w;
        }
    });
    total
}

/// Section 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k)).
pub fn european_price_sum(s0: f64, u: f64, d: f64, r: f64, num_steps: i32, payoff: Payoff) -> f64 {
    let mut total = 0.0;
    for_each_binomial_weight(risk_neutral_prob(u, d, r), num_steps, |k, w| {
        total += w * payoff(stock_price(s0, u, d, num_steps, k));
    });
    total * (1.0 + r).powf(f64::from(-num_steps))
}

/// Section 8.1.4: the result of the Cox-Ross-Rubinstein formula.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CrrCallResult {
    /// The call price.
    pub price: f64,
    /// The least number of up moves for which S(N) > X.
    pub m: i32,
}

/// Section 8.1.4: the Cox-Ross-Rubinstein formula,
/// C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*), with m the least
/// number of up moves for which S(N) > X and p' = p* (1+u) / (1+r).
pub fn crr_call(s0: f64, strike: f64, u: f64, d: f64, r: f64, num_steps: i32) -> CrrCallResult {
    let q = risk_neutral_prob(u, d, r);
    let mut m = 0;
    while m <= num_steps && stock_price(s0, u, d, num_steps, m) <= strike {
        m += 1;
    }
    let q_prime = q * (1.0 + u) / (1.0 + r);
    CrrCallResult {
        price: s0 * binomial_sum(q_prime, num_steps, m)
            - strike * (1.0 + r).powf(f64::from(-num_steps)) * binomial_sum(q, num_steps, m),
        m,
    }
}

/// Section 8.2: American options. Nodes (n, k), n < N, where exercising
/// beats waiting.
pub fn early_exercise_nodes(
    s0: f64,
    u: f64,
    d: f64,
    r: f64,
    num_steps: i32,
    payoff: Payoff,
) -> Vec<(i32, i32)> {
    let q = risk_neutral_prob(u, d, r);
    let values = option_tree(s0, u, d, r, num_steps, payoff, true);
    let price = StockPrices::new(s0, u, d, num_steps);
    let mut nodes = Vec::new();
    for n in 0..num_steps as usize {
        for k in 0..n + 1 {
            let wait = (q * values[n + 1][k + 1] + (1.0 - q) * values[n + 1][k]) / (1.0 + r);
            if payoff(price.at(n, k)) > wait {
                nodes.push((n as i32, k as i32));
            }
        }
    }
    nodes
}
