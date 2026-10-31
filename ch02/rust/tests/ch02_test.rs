//! Unit tests for chapter 2; each mirrors a C++, Kotlin and Python test.

use m4f_ch02::interest::{
    annuity_factor, annuity_future_value, annuity_present_value, continuous_effective_rate,
    continuous_from_periodic, continuous_value, effective_rate, loan_installment,
    periodic_from_continuous, periodic_from_periodic, periodic_value, simple_present_value,
    simple_rate, simple_value,
};
use m4f_ch02::money_market::{
    coupon_bond_price, implied_rate, money_market_rollover, yield_to_maturity, zero_coupon_price,
};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const EPS: f64 = 1e-9;

#[test]
fn library_identities() {
    assert!(
        near(
            simple_present_value(simple_value(100.0, 0.1, 0.5), 0.1, 0.5),
            100.0,
            EPS
        ),
        "simple PV"
    );
    assert!(
        near(
            simple_rate(100.0, simple_value(100.0, 0.08, 2.0), 2.0),
            0.08,
            EPS
        ),
        "simple rate"
    );
    assert!(near(effective_rate(0.1, 1), 0.1, EPS), "effective m=1");
    assert!(
        near(
            periodic_from_continuous(continuous_from_periodic(0.1, 12), 12),
            0.1,
            EPS
        ),
        "rate roundtrip"
    );
    assert!(
        near(
            periodic_from_periodic(periodic_from_periodic(0.1, 4, 12), 12, 4),
            0.1,
            EPS
        ),
        "m->k->m"
    );
    assert!(
        near(
            continuous_effective_rate(continuous_from_periodic(0.1, 1)),
            0.1,
            EPS
        ),
        "continuous effective"
    );
    assert!(
        near(
            periodic_value(100.0, 0.1, 1_000_000, 1.0),
            continuous_value(100.0, 0.1, 1.0),
            1e-4
        ),
        "continuous limit"
    );
    assert!(
        near(
            annuity_present_value(loan_installment(1000.0, 0.1, 5), 0.1, 5),
            1000.0,
            EPS
        ),
        "loan"
    );
    assert!(
        near(
            annuity_future_value(1.0, 0.1, 5),
            annuity_factor(0.1, 5) * 1.1f64.powf(5.0),
            EPS
        ),
        "FV"
    );
    for m in [Some(1), Some(2), Some(12), None] {
        assert!(
            near(
                implied_rate(zero_coupon_price(0.08, 2.5, m), 2.5, m),
                0.08,
                EPS
            ),
            "implied"
        );
    }
    assert!(
        near(
            yield_to_maturity(coupon_bond_price(100.0, 10.0, 3, 0.12), 100.0, 10.0, 3),
            0.12,
            1e-8
        ),
        "YTM"
    );
    assert!(
        near(coupon_bond_price(100.0, 10.0, 3, 0.10), 100.0, EPS),
        "par bond"
    );
}

/// The closed-form price and the Newton solver against a direct sum,
/// including a zero and a tiny yield and a price out of range.
#[test]
fn closed_form_and_newton() {
    for n in [1, 3, 60, 360] {
        for y in [-0.5_f64, -1e-9, 0.0, 1e-9, 0.004, 0.12, 2.0] {
            let mut direct = 100.0 * (1.0 + y).powf(f64::from(-n));
            for k in 1..=n {
                direct += 5.0 * (1.0 + y).powf(f64::from(-k));
            }
            assert!(
                near(coupon_bond_price(100.0, 5.0, n, y), direct, 1e-9 * direct),
                "closed-form coupon bond price, n={n} y={y}"
            );
            assert!(
                near(yield_to_maturity(direct, 100.0, 5.0, n), y, 1e-9),
                "YTM round trip"
            );
        }
    }
    assert!(
        near(yield_to_maturity(1e9, 100.0, 5.0, 3), -0.99, 1e-9),
        "YTM clamps to lo"
    );
}

#[test]
fn report_values() {
    assert!(
        near(annuity_present_value(1000.0, 0.10, 10), 6144.567105, 1e-6),
        "annuity PV"
    );
    assert!(
        near(loan_installment(100000.0, 0.005, 300), 644.301401, 1e-6),
        "installment"
    );
    assert!(
        near(continuous_from_periodic(0.12, 12), 0.119404, 1e-6),
        "12% monthly"
    );
    assert!(
        near(money_market_rollover(1.0, &[0.5, 0.5]), 4.0, EPS),
        "rollover"
    );
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch02::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
