//! Section 6.2, futures: pricing in the binomial model, marking to market,
//! and hedging with futures.

/// Section 6.2.1: pricing. In the binomial tree (`s0`, `u`, `d`, `r` per
/// step) the futures price for delivery at N is f(n) = E*(S(N) | S(n)):
/// f(N) = S(N) and backwards f(n, k) = p* f(n+1, k+1) + (1 - p*) f(n+1, k).
/// Returns `f[n][k]` after k up moves out of n.
pub fn futures_tree(s0: f64, u: f64, d: f64, r: f64, num_steps: i32) -> Vec<Vec<f64>> {
    let q = (r - d) / (u - d);
    let steps = num_steps as usize;
    let mut f: Vec<Vec<f64>> = vec![Vec::new(); steps + 1];
    f[steps] = (0..num_steps + 1)
        .map(|k| s0 * (1.0 + u).powf(f64::from(k)) * (1.0 + d).powf(f64::from(num_steps - k)))
        .collect();
    for n in (0..steps).rev() {
        let next = &f[n + 1];
        let row: Vec<f64> = (0..n + 1)
            .map(|k| q * next[k + 1] + (1.0 - q) * next[k])
            .collect();
        f[n] = row;
    }
    f
}

/// One day of a margin account.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct MarginDay {
    /// After settlement, before any call.
    pub balance: f64,
    /// Margin call that restores the initial margin.
    pub call: f64,
}

/// Marking to market for one long contract on `size` units: each day the
/// account gains size * (f(n) - f(n-1)); when it falls below the
/// maintenance margin, a call restores it to the initial margin.
pub fn margin_account(prices: &[f64], size: f64, initial: f64, maintenance: f64) -> Vec<MarginDay> {
    let mut balance = initial;
    prices
        .windows(2)
        .map(|w| {
            balance += size * (w[1] - w[0]);
            let call = if balance < maintenance {
                initial - balance
            } else {
                0.0
            };
            let day = MarginDay { balance, call };
            balance += call;
            day
        })
        .collect()
}

/// Section 6.2.2: hedging with futures. Selling h futures per unit of spot
/// exposure turns a spot change dS into dS - h df. In scenario i, with
/// probability `p[i]`, the spot price changes by `ds[i]` and the futures
/// price by `df[i]`. Returns h* = Cov(dS, df) / Var(df).
pub fn min_variance_hedge_ratio(ds: &[f64], df: &[f64], p: &[f64]) -> f64 {
    // Slicing all three to the same length up front lets the compiler drop
    // the bounds checks in both loops.
    let n = p.len();
    let (ds, df) = (&ds[..n], &df[..n]);
    let mut mean_s = 0.0;
    let mut mean_f = 0.0;
    for i in 0..p.len() {
        mean_s += p[i] * ds[i];
        mean_f += p[i] * df[i];
    }
    let mut cov = 0.0;
    let mut var = 0.0;
    for i in 0..p.len() {
        cov += p[i] * (ds[i] - mean_s) * (df[i] - mean_f);
        var += p[i] * (df[i] - mean_f) * (df[i] - mean_f);
    }
    cov / var
}

/// Standard deviation of dS - h df.
pub fn hedged_std(h: f64, ds: &[f64], df: &[f64], p: &[f64]) -> f64 {
    let x: Vec<f64> = ds.iter().zip(df).map(|(s, f)| s - h * f).collect();
    let p = &p[..x.len()];
    let mut m = 0.0;
    for i in 0..x.len() {
        m += p[i] * x[i];
    }
    let mut v = 0.0;
    for i in 0..x.len() {
        v += p[i] * (x[i] - m) * (x[i] - m);
    }
    v.sqrt()
}
