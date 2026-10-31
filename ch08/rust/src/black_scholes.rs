//! Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree
//! that converges to it (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma
//! sqrt(tau)), 1+r = e^(r tau), tau = T/N). r is the continuously
//! compounded rate, `s` the stock price now, `strike` the strike X, `sigma`
//! the volatility and `maturity` the time T to expiry in years.

use std::cell::Cell;

use crate::binomial_pricing::{StockPrices, risk_neutral_prob};
use crate::max;

/// Standard normal CDF to double precision (Hart 1968, as given by G. West,
/// "Better approximations to cumulative normal functions").
#[allow(clippy::excessive_precision)]
pub fn norm_cdf(x: f64) -> f64 {
    let a = x.abs();
    let c = if a > 37.0 {
        0.0
    } else {
        let e = (-a * a / 2.0).exp();
        if a < 7.07106781186547 {
            let mut b = 3.52624965998911e-02 * a + 0.700383064443688;
            b = b * a + 6.37396220353165;
            b = b * a + 33.912866078383;
            b = b * a + 112.079291497871;
            b = b * a + 221.213596169931;
            b = b * a + 220.206867912376;
            let c = e * b;
            b = 8.83883476483184e-02 * a + 1.75566716318264;
            b = b * a + 16.064177579207;
            b = b * a + 86.7807322029461;
            b = b * a + 296.564248779674;
            b = b * a + 637.333633378831;
            b = b * a + 793.826512519948;
            b = b * a + 440.413735824752;
            c / b
        } else {
            let mut b = a + 0.65;
            b = a + 4.0 / b;
            b = a + 3.0 / b;
            b = a + 2.0 / b;
            b = a + 1.0 / b;
            e / b / 2.506628274631
        }
    };
    if x > 0.0 { 1.0 - c } else { c }
}

/// (d1, d2) with d1 = (ln(S/X) + (r + sigma^2/2) T) / (sigma sqrt(T)) and
/// d2 = d1 - sigma sqrt(T).
pub fn d1_d2(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64) -> (f64, f64) {
    let d1 = ((s / strike).ln() + (r + sigma * sigma / 2.0) * maturity) / (sigma * maturity.sqrt());
    (d1, d1 - sigma * maturity.sqrt())
}

/// C = S N(d1) - X e^(-rT) N(d2).
pub fn bs_call(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64) -> f64 {
    let (d1, d2) = d1_d2(s, strike, r, sigma, maturity);
    s * norm_cdf(d1) - strike * (-r * maturity).exp() * norm_cdf(d2)
}

/// P = X e^(-rT) N(-d2) - S N(-d1).
pub fn bs_put(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64) -> f64 {
    let (d1, d2) = d1_d2(s, strike, r, sigma, maturity);
    strike * (-r * maturity).exp() * norm_cdf(-d2) - s * norm_cdf(-d1)
}

/// Shares in the replicating portfolio: N(d1).
pub fn bs_call_delta(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64) -> f64 {
    norm_cdf(d1_d2(s, strike, r, sigma, maturity).0)
}

/// CRR tree price of a European (or, with `american`, American) call or
/// put. (C++ defaults `american` to false.)
#[allow(clippy::too_many_arguments)]
pub fn crr_tree_price(
    s: f64,
    strike: f64,
    r: f64,
    sigma: f64,
    maturity: f64,
    num_steps: i32,
    is_call: bool,
    american: bool,
) -> f64 {
    let tau = maturity / f64::from(num_steps);
    let u = (sigma * tau.sqrt()).exp() - 1.0;
    let d = (-sigma * tau.sqrt()).exp() - 1.0;
    let r_step = (r * tau).exp() - 1.0;
    // The same backward induction as `option_tree`, keeping one row instead
    // of the whole tree and calling the payoff directly. Each payoff gets
    // its own copy of the loop, as the C++ compiler does after unswitching
    // on `is_call`.
    let q = risk_neutral_prob(u, d, r_step);
    let price = StockPrices::new(s, u, d, num_steps);
    if is_call {
        backward_induction(&price, num_steps, q, r_step, american, |x| {
            max(x - strike, 0.0)
        })
    } else {
        backward_induction(&price, num_steps, q, r_step, american, |x| {
            max(strike - x, 0.0)
        })
    }
}

/// Rolls one row of option values back from step N to the root.
fn backward_induction(
    price: &StockPrices,
    num_steps: i32,
    q: f64,
    r: f64,
    american: bool,
    payoff: impl Fn(f64) -> f64,
) -> f64 {
    let steps = num_steps as usize;
    let mut values: Vec<f64> = (0..steps + 1).map(|k| payoff(price.at(steps, k))).collect();
    for n in (0..steps).rev() {
        // Slicing to this row's length up front lets the compiler drop the
        // bounds checks inside the loop.
        let row = &mut values[..n + 2];
        if american {
            // Exercise value S(0) up[k] down[n-k] from the two power tables,
            // walked with iterators: indexing down[n - k] would keep a bounds
            // check in the loop.
            let (up, down) = price.row_factors(n);
            let cells = Cell::from_mut(row).as_slice_of_cells();
            for ((pair, up_k), down_n_k) in cells.windows(2).zip(up).zip(down.iter().rev()) {
                let wait = (q * pair[1].get() + (1.0 - q) * pair[0].get()) / (1.0 + r);
                pair[0].set(max(wait, payoff(price.s0() * up_k * down_n_k)));
            }
        } else {
            for k in 0..n + 1 {
                row[k] = (q * row[k + 1] + (1.0 - q) * row[k]) / (1.0 + r);
            }
        }
    }
    values[0]
}
