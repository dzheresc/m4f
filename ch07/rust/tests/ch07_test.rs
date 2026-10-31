//! Unit tests for chapter 7; each mirrors a C++, Kotlin and Python test.

use m4f_ch07::binomial::crr_price;
use m4f_ch07::options::{
    Bounds, american_parity_bounds, bound_violation, butterfly_gap, call_payoff,
    european_call_bounds, european_put_bounds, parity_gap, profit, put_payoff,
};

fn near(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

fn price(s0: f64, strike: f64, maturity: f64, is_call: bool, american: bool) -> f64 {
    crr_price(s0, strike, 0.05, 0.2, maturity, 200, is_call, american)
}

/// Section 7.1.
#[test]
fn payoffs() {
    for s in [80.0, 100.0, 120.0] {
        assert!(
            near(call_payoff(s, 100.0) - put_payoff(s, 100.0), s - 100.0),
            "payoffs"
        );
    }
    assert!(
        near(profit(0.0, 10.0, 0.05, 1.0), -10.0 * 0.05f64.exp()),
        "profit"
    );
}

/// Section 7.2.
#[test]
fn parity() {
    let call = price(100.0, 100.0, 1.0, true, false);
    let put = price(100.0, 100.0, 1.0, false, false);
    assert!(
        near(parity_gap(call, put, 100.0, 100.0, 0.05, 1.0, 0.0), 0.0),
        "parity"
    );
    assert!(
        near(
            parity_gap(12.0, 5.0, 100.0, 100.0, 0.05, 1.0, 0.0),
            7.0 - 100.0 + 100.0 * (-0.05f64).exp()
        ),
        "parity gap"
    );
}

/// Section 7.3.
#[test]
fn bounds() {
    for s0 in [80.0, 100.0, 120.0] {
        for maturity in [0.25, 1.0] {
            let calls = european_call_bounds(s0, 100.0, 0.05, maturity, 0.0);
            let puts = european_put_bounds(s0, 100.0, 0.05, maturity, 0.0);
            assert_eq!(
                bound_violation(price(s0, 100.0, maturity, true, false), calls),
                0.0
            );
            assert_eq!(
                bound_violation(price(s0, 100.0, maturity, false, false), puts),
                0.0
            );
            let b = american_parity_bounds(s0, 100.0, 0.05, maturity);
            let d =
                price(s0, 100.0, maturity, true, true) - price(s0, 100.0, maturity, false, true);
            assert!(b.lo <= d && d <= b.hi, "American parity bounds");
        }
    }
    let b = Bounds { lo: 4.0, hi: 100.0 };
    assert!(near(bound_violation(3.5, b), 0.5) && near(bound_violation(101.0, b), 1.0));
}

/// Sections 7.3.2 and 7.4.2.
#[test]
fn american() {
    for s0 in [80.0, 100.0, 120.0] {
        assert!(near(
            price(s0, 100.0, 1.0, true, true),
            price(s0, 100.0, 1.0, true, false)
        ));
        assert!(price(s0, 100.0, 1.0, false, true) >= price(s0, 100.0, 1.0, false, false));
        assert!(price(s0, 100.0, 1.0, false, true) >= put_payoff(s0, 100.0));
    }
    let mut prev = 0.0;
    for maturity in [0.25, 0.5, 1.0, 2.0] {
        let p = price(100.0, 100.0, maturity, false, true);
        assert!(p >= prev, "P_A increasing in T");
        prev = p;
    }
}

/// Section 7.4.1.
#[test]
fn strike() {
    let c1 = price(100.0, 90.0, 1.0, true, false);
    let c2 = price(100.0, 100.0, 1.0, true, false);
    let c3 = price(100.0, 110.0, 1.0, true, false);
    assert!(c1 > c2 && c2 > c3, "C decreasing in X");
    assert!(
        butterfly_gap(c1, c2, c3, 90.0, 100.0, 110.0) <= 0.0,
        "convex in X"
    );
    assert!(c1 - c2 <= 10.0 * (-0.05f64).exp(), "slope in X");
    assert!(
        near(butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0), 1.5),
        "butterfly"
    );
}

/// `crr_price` against a direct backward induction that computes every
/// node's stock price with `powf`, as the book writes it.
#[allow(clippy::too_many_arguments)]
fn reference_crr(
    s0: f64,
    strike: f64,
    r: f64,
    sigma: f64,
    maturity: f64,
    num_steps: i32,
    is_call: bool,
    american: bool,
) -> f64 {
    let tau = maturity / f64::from(num_steps);
    let up = (sigma * tau.sqrt()).exp();
    let down = 1.0 / up;
    let growth = (r * tau).exp();
    let q = (growth - down) / (up - down);
    let payoff = |s: f64| {
        if is_call {
            (s - strike).max(0.0)
        } else {
            (strike - s).max(0.0)
        }
    };
    let price = |n: i32, k: i32| s0 * up.powf(f64::from(k)) * down.powf(f64::from(n - k));
    let mut v: Vec<f64> = (0..=num_steps)
        .map(|k| payoff(price(num_steps, k)))
        .collect();
    for n in (0..num_steps).rev() {
        for k in 0..=n {
            let i = k as usize;
            v[i] = (q * v[i + 1] + (1.0 - q) * v[i]) / growth;
            if american {
                v[i] = v[i].max(payoff(price(n, k)));
            }
        }
    }
    v[0]
}

#[test]
fn crr_matches_reference() {
    for n in [1, 7, 200, 1000] {
        for is_call in [true, false] {
            for american in [false, true] {
                for strike in [80.0, 100.0, 125.0] {
                    assert!(
                        near(
                            crr_price(100.0, strike, 0.05, 0.3, 1.5, n, is_call, american),
                            reference_crr(100.0, strike, 0.05, 0.3, 1.5, n, is_call, american)
                        ),
                        "crr_price matches the pow-per-node reference"
                    );
                }
            }
        }
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch07::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
