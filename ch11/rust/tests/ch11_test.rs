//! Unit tests for chapter 11; each mirrors a C++, Kotlin and Python test.

use m4f_ch11::derivatives::{
    bond_option, cap, caplet, floating_rate, floor, payer_swap, price_payments, swap_rate,
};
use m4f_ch11::rate_tree::{
    Lattice, bond_lattice, bond_prices, constant_probs, discount_back, implied_prob, implied_probs,
    money_market, one_step_price, rate_lattice, yield_rate,
};

fn near(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

const NUM_STEPS: i32 = 4;
const TAU: f64 = 1.0;

fn tree() -> (Lattice, Lattice) {
    (
        rate_lattice(0.05, 0.01, -0.01, NUM_STEPS),
        constant_probs(0.5, NUM_STEPS),
    )
}

/// Section 11.1.
#[test]
fn rate_tree() {
    let (rates, _) = tree();
    assert!(
        rates.len() == 5 && near(rates[2][2], 0.07) && near(rates[2][0], 0.03),
        "rate lattice"
    );
    let account = money_market(&rates, TAU, "ud");
    assert!(
        near(account[1], 0.05_f64.exp()) && near(account[2], (0.05_f64 + 0.06).exp()),
        "money market"
    );
    assert!(
        near(one_step_price(0.05, 0.5), (-0.025_f64).exp()),
        "one-step price"
    );
}

/// Section 11.2.
#[test]
fn bond_pricing() {
    let (rates, probs) = tree();
    let b1 = bond_lattice(&rates, &probs, TAU, 1);
    assert!(near(b1[0][0], (-0.05_f64).exp()), "B(0,1)");
    let b2 = bond_lattice(&rates, &probs, TAU, 2);
    assert!(
        near(
            b2[0][0],
            (-0.05_f64).exp() * (0.5 * (-0.06_f64).exp() + 0.5 * (-0.04_f64).exp())
        ),
        "B(0,2)"
    );
    let b0 = bond_prices(&rates, &probs, TAU, NUM_STEPS);
    assert!(b0.len() == 4, "4 prices");
    for m in 1..NUM_STEPS + 1 {
        assert!(
            near(
                b0[m as usize - 1],
                bond_lattice(&rates, &probs, TAU, m)[0][0]
            ),
            "bond prices"
        );
    }
    for w in b0.windows(2) {
        assert!(w[0] > w[1], "decreasing in maturity");
    }
    assert!(near(yield_rate(b1[0][0], 0, 1, TAU), 0.05), "yield");
    let b4 = bond_lattice(&rates, &probs, TAU, NUM_STEPS);
    for n in 0..NUM_STEPS as usize {
        for k in 0..n + 1 {
            assert!(
                near(
                    b4[n][k] * (rates[n][k] * TAU).exp(),
                    0.5 * b4[n + 1][k + 1] + 0.5 * b4[n + 1][k]
                ),
                "martingale"
            );
        }
    }
}

/// Section 11.2.1.
#[test]
fn implied() {
    let (rates, probs) = tree();
    for p in [0.5, 0.6, 0.35] {
        let bond = bond_lattice(&rates, &constant_probs(p, NUM_STEPS), TAU, NUM_STEPS);
        for row in implied_probs(&bond, &rates, TAU) {
            for q in row {
                assert!(near(q, p), "implied p*");
            }
        }
    }
    let b2 = bond_lattice(&rates, &probs, TAU, 2);
    assert!(
        near(implied_prob(&b2, &rates, TAU, 0, 0), 0.5),
        "implied p* from B2"
    );
}

/// Section 11.3.1.
#[test]
fn bond_options() {
    let (rates, probs) = tree();
    let b2 = bond_lattice(&rates, &probs, TAU, 2);
    let b4 = bond_lattice(&rates, &probs, TAU, NUM_STEPS);
    let call = bond_option(&rates, &probs, TAU, NUM_STEPS, 2, 0.9, true, false);
    let put = bond_option(&rates, &probs, TAU, NUM_STEPS, 2, 0.9, false, false);
    assert!(
        near(call[0][0] - put[0][0], b4[0][0] - 0.9 * b2[0][0]),
        "put-call parity"
    );
    let american_put = bond_option(&rates, &probs, TAU, NUM_STEPS, 2, 0.9, false, true);
    assert!(
        american_put[0][0] >= put[0][0] && near(american_put[0][0], 0.9 - b4[0][0]),
        "American put"
    );
    assert!(
        near(
            bond_option(&rates, &probs, TAU, NUM_STEPS, 2, 0.0, true, false)[0][0],
            b4[0][0]
        ),
        "zero-strike call = bond"
    );
}

/// Sections 11.3.2-11.3.3.
#[test]
fn swaps_caps_floors() {
    let (rates, probs) = tree();
    let b0 = bond_prices(&rates, &probs, TAU, NUM_STEPS);
    let k_swap = swap_rate(&b0);
    assert!(
        near(
            payer_swap(&rates, &probs, TAU, NUM_STEPS, k_swap)[0][0],
            0.0
        ),
        "swap worthless at swap rate"
    );
    assert!(
        near(
            payer_swap(&rates, &probs, TAU, NUM_STEPS, 0.0)[0][0],
            1.0 - b0[b0.len() - 1]
        ),
        "floating leg = 1 - B(0,N)"
    );
    for strike in [0.03, k_swap, 0.07] {
        let c = cap(&rates, &probs, TAU, NUM_STEPS, strike)[0][0];
        let f = floor(&rates, &probs, TAU, NUM_STEPS, strike)[0][0];
        assert!(
            near(
                c - f,
                payer_swap(&rates, &probs, TAU, NUM_STEPS, strike)[0][0]
            ),
            "cap - floor = swap"
        );
        assert!(c >= 0.0 && f >= 0.0, "nonnegative");
    }
    let mut sum = 0.0;
    for n in 0..NUM_STEPS {
        sum += caplet(&rates, &probs, TAU, n, 0.05);
    }
    assert!(
        near(sum, cap(&rates, &probs, TAU, NUM_STEPS, 0.05)[0][0]),
        "caplets sum to cap"
    );
    assert!(
        near(
            caplet(&rates, &probs, TAU, 0, 0.05),
            (-0.05_f64).exp() * (floating_rate(&rates, TAU, 0, 0) - 0.05).max(0.0)
        ),
        "first caplet"
    );
    let annuity = discount_back(
        &rates,
        &probs,
        TAU,
        vec![0.0; NUM_STEPS as usize + 1],
        Some(&|_, _| 1.0),
        None,
    );
    assert!(
        near(annuity[0][0], b0.iter().sum::<f64>()),
        "annuity = sum of bonds"
    );
}

/// The forward-induction `bond_prices` and the rolling `caplet` against the
/// lattice computations they replaced, with node-dependent probabilities.
#[test]
fn against_reference() {
    const N: i32 = 60;
    let rates = rate_lattice(0.05, 0.004, -0.003, N);
    let probs: Lattice = (0..N)
        .map(|n| {
            (0..n + 1)
                .map(|k| 0.3 + 0.4 * ((0.618 * f64::from(n * 7 + k * 3)) % 1.0))
                .collect()
        })
        .collect();
    let b0 = bond_prices(&rates, &probs, 0.25, N);
    for m in 1..N + 1 {
        let reference = bond_lattice(&rates, &probs, 0.25, m)[0][0];
        assert!(
            (b0[m as usize - 1] - reference).abs() <= 1e-14 * reference,
            "bond_prices vs lattice"
        );
    }
    for fixing in [0, 1, 7, 59] {
        let payment = |n: i32, k: i32| {
            if n == fixing {
                (floating_rate(&rates, 0.25, n, k) - 0.05).max(0.0)
            } else {
                0.0
            }
        };
        let reference = price_payments(&rates, &probs, 0.25, fixing + 1, &payment)[0][0];
        assert!(
            caplet(&rates, &probs, 0.25, fixing, 0.05) == reference,
            "caplet vs price_payments"
        );
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch11::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
