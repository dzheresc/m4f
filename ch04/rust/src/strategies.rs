//! Section 4.1, stock and money market models: investment strategies.
//!
//! Prices are given along one path: `s[n]` (or `s[j][n]` for several
//! stocks) and `a[n]` for n = 0..N, standing for the book's S(n) and A(n). A
//! strategy holds x(n) shares and y(n) units of the money market over the
//! step from n-1 to n, chosen at time n-1; vectors are indexed from 0, so
//! `x[0]` is x(1).

/// Money market positions and values of a strategy along a path.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct StrategyPath {
    /// `y[n-1]` = y(n).
    pub y: Vec<f64>,
    /// `values[n]` = V(n).
    pub values: Vec<f64>,
}

/// Several stocks `s[j][n]`, optional dividends `dividends[j][n]` paid at n
/// to the holders over (n-1, n]. The money market position y(n+1) is set
/// so that rebalancing at n costs exactly V(n):
///
/// ```text
/// sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
/// V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).
/// ```
pub fn self_financing_multi(
    v0: f64,
    s: &[Vec<f64>],
    a: &[f64],
    x: &[Vec<f64>],
    dividends: Option<&[Vec<f64>]>,
) -> StrategyPath {
    let num_steps = a.len() - 1;
    let mut path = StrategyPath {
        y: Vec::with_capacity(num_steps),
        values: Vec::with_capacity(num_steps + 1),
    };
    path.values.push(v0);
    for n in 1..num_steps + 1 {
        let mut stock = 0.0;
        for (x_j, s_j) in x.iter().zip(s) {
            stock += x_j[n - 1] * s_j[n - 1];
        }
        let y = (path.values[n - 1] - stock) / a[n - 1];
        path.y.push(y);
        let mut value = y * a[n];
        match dividends {
            Some(div) => {
                for ((x_j, s_j), div_j) in x.iter().zip(s).zip(div) {
                    value += x_j[n - 1] * (s_j[n] + div_j[n]);
                }
            }
            None => {
                for (x_j, s_j) in x.iter().zip(s) {
                    value += x_j[n - 1] * (s_j[n] + 0.0);
                }
            }
        }
        path.values.push(value);
    }
    path
}

/// One stock: y(n) and V(n) of the self-financing strategy with initial
/// value `v0` and stock positions `x`. Same arithmetic as
/// `self_financing_multi` with one stock and no dividends, without copying
/// `s` and `x` into matrices.
pub fn self_financing(v0: f64, s: &[f64], a: &[f64], x: &[f64]) -> StrategyPath {
    let num_steps = a.len() - 1;
    let mut path = StrategyPath {
        y: Vec::with_capacity(num_steps),
        values: Vec::with_capacity(num_steps + 1),
    };
    path.values.push(v0);
    for n in 1..num_steps + 1 {
        let y = (path.values[n - 1] - x[n - 1] * s[n - 1]) / a[n - 1];
        path.y.push(y);
        path.values.push(y * a[n] + x[n - 1] * s[n]);
    }
    path
}

/// V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1)).
pub fn gains(s: &[f64], a: &[f64], x: &[f64], y: &[f64]) -> f64 {
    // Iterating over pairs of neighbours instead of indexing leaves no
    // bounds checks in the loop (the C++ indexes without checks). The sum
    // runs in the same order.
    s.windows(2)
        .zip(a.windows(2))
        .zip(x.iter().zip(y))
        .fold(0.0, |gains, ((s, a), (x, y))| {
            gains + (x * (s[1] - s[0]) + y * (a[1] - a[0]))
        })
}

/// V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n).
pub fn discounted_gains(s: &[f64], a: &[f64], x: &[f64]) -> f64 {
    s.windows(2)
        .zip(a.windows(2))
        .zip(x)
        .fold(0.0, |gains, ((s, a), x)| {
            gains + x * (s[1] / a[1] - s[0] / a[0])
        })
}
