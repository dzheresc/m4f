//! Unit tests for chapter 8; each mirrors a C++, Kotlin and Python test.

use m4f_ch08::binomial_pricing::{
    binomial_sum, crr_call, early_exercise_nodes, european_price_sum, one_step_replication,
    option_tree, replicating_position, stock_price,
};
use m4f_ch08::black_scholes::{bs_call, bs_call_delta, bs_put, crr_tree_price, norm_cdf};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

fn call(strike: f64) -> impl Fn(f64) -> f64 {
    move |s| (s - strike).max(0.0)
}

fn put(strike: f64) -> impl Fn(f64) -> f64 {
    move |s| (strike - s).max(0.0)
}

/// Section 8.1.1.
#[test]
fn one_step() {
    let rep = one_step_replication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0);
    assert!(
        near(rep.x * 120.0 + rep.y * 1.1, 20.0, 1e-9)
            && near(rep.x * 90.0 + rep.y * 1.1, 0.0, 1e-9),
        "one-step replication"
    );
    assert!(near(rep.price, 40.0 / 3.3, 1e-9), "one-step price");
}

/// Section 8.1.2: the replicating position reproduces both successor values.
#[test]
fn replication() {
    let values = option_tree(100.0, 0.2, -0.1, 0.1, 3, &call(100.0), false);
    for n in 0..3 {
        for k in 0..n + 1 {
            let (x, y) = replicating_position(100.0, 0.2, -0.1, 0.1, &values, n, k);
            for next_k in [k, k + 1] {
                assert!(
                    near(
                        x * stock_price(100.0, 0.2, -0.1, n + 1, next_k)
                            + y * 1.1_f64.powf(f64::from(n + 1)),
                        values[n as usize + 1][next_k as usize],
                        1e-9
                    ),
                    "replication"
                );
            }
        }
    }
}

/// Sections 8.1.3-8.1.4.
#[test]
fn formulas() {
    for num_steps in [1, 2, 5, 8] {
        for strike in [70.0, 100.0, 130.0] {
            let c = option_tree(100.0, 0.2, -0.1, 0.1, num_steps, &call(strike), false)[0][0];
            assert!(
                near(
                    european_price_sum(100.0, 0.2, -0.1, 0.1, num_steps, &call(strike)),
                    c,
                    1e-9
                ),
                "sum formula"
            );
            assert!(
                near(
                    crr_call(100.0, strike, 0.2, -0.1, 0.1, num_steps).price,
                    c,
                    1e-9
                ),
                "CRR formula"
            );
            let p = option_tree(100.0, 0.2, -0.1, 0.1, num_steps, &put(strike), false)[0][0];
            assert!(
                near(
                    c - p,
                    100.0 - strike * 1.1_f64.powf(f64::from(-num_steps)),
                    1e-9
                ),
                "put-call parity"
            );
        }
    }
    assert!(near(binomial_sum(0.3, 6, 0), 1.0, 1e-9), "binomial sum");
    // Large N: C(2000, 1000) alone overflows a double.
    const BIG: i32 = 2000;
    let tau = 1.0 / f64::from(BIG);
    let u = (0.2 * tau.sqrt()).exp() - 1.0;
    let d = (-0.2 * tau.sqrt()).exp() - 1.0;
    let r = (0.05 * tau).exp() - 1.0;
    assert!(
        near(binomial_sum(0.5, BIG, 0), 1.0, 1e-9),
        "large-N binomial sum"
    );
    let crr = crr_call(100.0, 100.0, u, d, r, BIG).price;
    assert!(
        near(
            crr,
            european_price_sum(100.0, u, d, r, BIG, &call(100.0)),
            1e-9
        ),
        "large-N CRR formula = sum"
    );
    assert!(
        near(crr, bs_call(100.0, 100.0, 0.05, 0.2, 1.0), 1e-2),
        "large-N CRR formula near Black-Scholes"
    );
}

/// Section 8.2.
#[test]
fn american() {
    let american = option_tree(100.0, 0.1, -0.1, 0.02, 4, &put(100.0), true);
    let european = option_tree(100.0, 0.1, -0.1, 0.02, 4, &put(100.0), false);
    assert!(american[0][0] > european[0][0], "P_A > P_E");
    for n in 0..5 {
        for k in 0..n + 1 {
            assert!(
                american[n as usize][k as usize]
                    >= (100.0 - stock_price(100.0, 0.1, -0.1, n, k)).max(0.0) - 1e-12,
                "P_A >= payoff"
            );
        }
    }
    assert!(
        early_exercise_nodes(100.0, 0.1, -0.1, 0.02, 4, &call(100.0)).is_empty(),
        "never exercise a call early"
    );
    assert!(
        near(
            option_tree(100.0, 0.1, -0.1, 0.02, 4, &call(100.0), true)[0][0],
            option_tree(100.0, 0.1, -0.1, 0.02, 4, &call(100.0), false)[0][0],
            1e-9
        ),
        "C_A = C_E"
    );
}

/// Section 8.3.
#[test]
fn black_scholes() {
    assert!(near(norm_cdf(0.0), 0.5, 1e-14), "N(0)");
    assert!(near(norm_cdf(1.959963984540054), 0.975, 1e-12), "N(1.96)");
    assert!(
        near(norm_cdf(-8.0) + norm_cdf(8.0), 1.0, 1e-14),
        "N symmetry"
    );
    let c = bs_call(100.0, 100.0, 0.05, 0.2, 1.0);
    let p = bs_put(100.0, 100.0, 0.05, 0.2, 1.0);
    assert!(near(c, 10.450583572185565, 1e-10), "BS call");
    assert!(
        near(c - p, 100.0 - 100.0 * (-0.05_f64).exp(), 1e-9),
        "BS parity"
    );
    let h = 1e-4;
    let finite_diff = (bs_call(100.0 + h, 100.0, 0.05, 0.2, 1.0)
        - bs_call(100.0 - h, 100.0, 0.05, 0.2, 1.0))
        / (2.0 * h);
    assert!(
        near(
            bs_call_delta(100.0, 100.0, 0.05, 0.2, 1.0),
            finite_diff,
            1e-6
        ),
        "delta"
    );
    assert!(
        near(
            crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, 2000, true, false),
            c,
            1e-2
        ),
        "CRR converges to BS"
    );
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch08::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
