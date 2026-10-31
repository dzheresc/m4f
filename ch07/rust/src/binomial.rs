//! Option prices used to illustrate Sections 7.3-7.5: the
//! Cox-Ross-Rubinstein tree with N steps of length tau = T/N,
//! u = e^(sigma sqrt(tau)), d = 1/u, growth e^(r tau) per step (Chapter 8
//! develops the model itself).

use std::cell::Cell;

use crate::max;

/// CRR price of a European (or, with `american`, American) call or put.
/// (C++ defaults `american` to false.)
#[allow(clippy::too_many_arguments)]
pub fn crr_price(
    s0: f64,
    strike: f64,
    r: f64,
    sigma: f64,
    maturity: f64,
    num_steps: i32,
    is_call: bool,
    american: bool,
) -> f64 {
    let n_steps = num_steps as usize;
    let tau = maturity / f64::from(num_steps);
    let up = (sigma * tau.sqrt()).exp();
    let down = 1.0 / up;
    let growth = (r * tau).exp();
    let q = (growth - down) / (up - down);
    // With down = 1/up the stock price at node (n, k) is s0 up^(2k - n), so
    // all node prices come from one table of 2N+1 powers instead of two
    // `powf` calls per node.
    let prices: Vec<f64> = (0..2 * num_steps + 1)
        .map(|j| s0 * up.powf(f64::from(j - num_steps)))
        .collect();
    // Each payoff gets its own copy of the loop, as the C++ compiler does
    // after unswitching on `is_call`.
    if is_call {
        backward_induction(&prices, n_steps, q, growth, american, |s| {
            max(s - strike, 0.0)
        })
    } else {
        backward_induction(&prices, n_steps, q, growth, american, |s| {
            max(strike - s, 0.0)
        })
    }
}

/// Rolls one row of option values back from step N to the root; node
/// (n, k) has stock price `prices[N - n + 2k]`.
fn backward_induction(
    prices: &[f64],
    n_steps: usize,
    q: f64,
    growth: f64,
    american: bool,
    payoff: impl Fn(f64) -> f64,
) -> f64 {
    // The prices of one row are every other table entry. Splitting the
    // table by parity makes each row a contiguous slice, which the loop
    // below can walk with plain iterators.
    let even: Vec<f64> = prices.iter().step_by(2).copied().collect();
    let odd: Vec<f64> = prices.iter().skip(1).step_by(2).copied().collect();
    let mut values: Vec<f64> = even.iter().map(|&s| payoff(s)).collect();
    for n in (0..n_steps).rev() {
        let row = &mut values[..n + 2];
        if american {
            // Walked with iterators rather than indices: indexing would keep
            // bounds checks in the loop and stop it from vectorising.
            let start = n_steps - n;
            let table = if start % 2 == 0 { &even } else { &odd };
            let node_prices = &table[start / 2..start / 2 + n + 1];
            let cells = Cell::from_mut(row).as_slice_of_cells();
            for (pair, &s) in cells.windows(2).zip(node_prices) {
                let wait = (q * pair[1].get() + (1.0 - q) * pair[0].get()) / growth;
                pair[0].set(max(wait, payoff(s)));
            }
        } else {
            for k in 0..n + 1 {
                row[k] = (q * row[k + 1] + (1.0 - q) * row[k]) / growth;
            }
        }
    }
    values[0]
}
