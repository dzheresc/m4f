//! Unit tests for chapter 6; each mirrors a C++, Kotlin and Python test.

use m4f_ch06::forwards::{
    forward_arbitrage_profit, forward_price, forward_price_dividends, forward_price_yield,
    long_forward_value,
};
use m4f_ch06::futures::{futures_tree, hedged_std, margin_account, min_variance_hedge_ratio};

fn near(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

/// Section 6.1.1.
#[test]
fn forward_prices() {
    assert!(
        near(forward_price(100.0, 0.08, 0.75), 100.0 * 0.06f64.exp()),
        "F = S e^(rT)"
    );
    assert!(
        near(
            forward_price_dividends(100.0, 0.08, 0.75, &[]),
            forward_price(100.0, 0.08, 0.75)
        ),
        "no dividends"
    );
    assert!(
        near(
            forward_price_yield(100.0, 0.08, 0.0, 0.75),
            forward_price(100.0, 0.08, 0.75)
        ),
        "zero yield"
    );
    assert!(
        near(
            forward_price_dividends(100.0, 0.08, 0.75, &[(2.0, 0.25), (2.0, 0.5)]),
            forward_price(100.0, 0.08, 0.75) - 2.0 * 0.04f64.exp() - 2.0 * 0.02f64.exp()
        ),
        "dividends"
    );
    assert!(
        near(
            forward_arbitrage_profit(forward_price(100.0, 0.08, 0.75), 100.0, 0.08, 0.75),
            0.0
        ),
        "fair forward"
    );
}

/// Section 6.1.2.
#[test]
fn forward_value() {
    let f0 = forward_price(100.0, 0.08, 0.75);
    assert!(
        near(long_forward_value(f0, f0, 0.08, 0.25), 0.0),
        "V = 0 at the same price"
    );
    // A long forward is a long stock plus a short bond paying f0 at T.
    assert!(
        near(
            long_forward_value(f0, forward_price(104.0, 0.08, 0.25), 0.08, 0.25),
            104.0 - f0 * (-0.08f64 * 0.25).exp()
        ),
        "long forward = stock - bond"
    );
}

/// Section 6.2.1.
#[test]
fn futures() {
    let tree = futures_tree(100.0, 0.10, -0.05, 0.03, 3);
    for n in 0..4 {
        for k in 0..n + 1 {
            let forward = 100.0
                * 1.1f64.powf(f64::from(k))
                * 0.95f64.powf(f64::from(n - k))
                * 1.03f64.powf(f64::from(3 - n));
            assert!(
                near(tree[n as usize][k as usize], forward),
                "futures = forward with constant r"
            );
        }
    }
    let prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0];
    let days = margin_account(&prices, 100.0, 1000.0, 750.0);
    let calls = [0.0, 500.0, 0.0, 0.0, 0.0];
    for (day, call) in days.iter().zip(calls) {
        assert!(near(day.call, call), "margin calls");
    }
    // Final balance = initial + calls + total gain.
    assert!(
        near(
            days.last().unwrap().balance,
            1000.0 + 500.0 + 100.0 * (102.0 - 106.0)
        ),
        "final balance"
    );
}

/// Section 6.2.2.
#[test]
fn hedging() {
    let ds = [-6.0, -1.0, 2.0, 5.0];
    let df = [-5.0, -2.0, 3.0, 4.0];
    let p = [0.25, 0.25, 0.25, 0.25];
    let h = min_variance_hedge_ratio(&ds, &df, &p);
    assert!(near(h, 29.0 / 27.0), "h*");
    assert!(
        hedged_std(h, &ds, &df, &p) < hedged_std(h - 0.05, &ds, &df, &p)
            && hedged_std(h, &ds, &df, &p) < hedged_std(h + 0.05, &ds, &df, &p),
        "h* minimises risk"
    );
    assert!(near(hedged_std(1.0, &ds, &ds, &p), 0.0), "perfect hedge");
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch06::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
