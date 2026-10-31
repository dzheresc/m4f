//! Section 11.3: interest rate derivatives on the binomial tree.
//!
//! Floating payments are the simple one-period rate fixed at (n,k) and paid
//! at n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional
//! of 1.

use crate::max;
use crate::rate_tree::{Lattice, NodeFn, bond_lattice, discount_back, one_step_price};

/// L(n,k) = 1/B(n,n+1;k) - 1.
#[inline]
pub fn floating_rate(rates: &[Vec<f64>], tau: f64, n: i32, k: i32) -> f64 {
    1.0 / one_step_price(rates[n as usize][k as usize], tau) - 1.0
}

/// 11.3.1 Options on a zero-coupon bond maturing at `maturity`, expiring
/// at `expiry` <= maturity. (C++ defaults `american` to false.)
#[allow(clippy::too_many_arguments)]
pub fn bond_option(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    maturity: i32,
    expiry: i32,
    strike: f64,
    is_call: bool,
    american: bool,
) -> Lattice {
    let bond = bond_lattice(rates, probs, tau, maturity);
    let payoff = |b: f64| {
        if is_call {
            max(b - strike, 0.0)
        } else {
            max(strike - b, 0.0)
        }
    };
    let terminal = bond[expiry as usize].iter().map(|&b| payoff(b)).collect();
    let exercise = |n: i32, k: i32| payoff(bond[n as usize][k as usize]);
    let early: Option<NodeFn> = if american { Some(&exercise) } else { None };
    discount_back(rates, probs, tau, terminal, None, early)
}

/// 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on
/// notional 1. The fixed rate making the swap worthless:
/// K sum B(0,n) = 1 - B(0,N).
pub fn swap_rate(bond_prices: &[f64]) -> f64 {
    let mut s = 0.0;
    for b in bond_prices {
        s += b;
    }
    (1.0 - bond_prices[bond_prices.len() - 1]) / s
}

/// Value lattice of a stream paying `payment(n,k)` at n+1 for n = 0..N-1.
pub fn price_payments(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    num_steps: i32,
    payment: NodeFn,
) -> Lattice {
    discount_back(
        rates,
        probs,
        tau,
        vec![0.0; num_steps as usize + 1],
        Some(payment),
        None,
    )
}

/// Receive floating, pay fixed `fixed_rate`.
pub fn payer_swap(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    num_steps: i32,
    fixed_rate: f64,
) -> Lattice {
    price_payments(rates, probs, tau, num_steps, &|n, k| {
        floating_rate(rates, tau, n, k) - fixed_rate
    })
}

/// 11.3.3 Caps: each caplet pays max(L(n,k) - K, 0).
pub fn cap(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    num_steps: i32,
    strike: f64,
) -> Lattice {
    price_payments(rates, probs, tau, num_steps, &|n, k| {
        max(floating_rate(rates, tau, n, k) - strike, 0.0)
    })
}

/// Floors: each floorlet pays max(K - L(n,k), 0).
pub fn floor(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    num_steps: i32,
    strike: f64,
) -> Lattice {
    price_payments(rates, probs, tau, num_steps, &|n, k| {
        max(strike - floating_rate(rates, tau, n, k), 0.0)
    })
}

/// Value at 0 of the single caplet fixed at `fixing` and paid at
/// fixing + 1. Only level `fixing` pays, so this rolls one row back from
/// there instead of building the whole `price_payments` lattice; the result
/// is the same.
pub fn caplet(rates: &[Vec<f64>], probs: &[Vec<f64>], tau: f64, fixing: i32, strike: f64) -> f64 {
    let f = fixing as usize;
    let mut v: Vec<f64> = (0..fixing + 1)
        .map(|k| {
            one_step_price(rates[f][k as usize], tau)
                * max(floating_rate(rates, tau, fixing, k) - strike, 0.0)
        })
        .collect();
    for n in (0..f).rev() {
        let (r_row, p_row, row) = (&rates[n][..n + 1], &probs[n][..n + 1], &mut v[..n + 2]);
        for k in 0..n + 1 {
            let p = p_row[k];
            row[k] = one_step_price(r_row[k], tau) * (p * row[k + 1] + (1.0 - p) * row[k]);
        }
    }
    v[0]
}
