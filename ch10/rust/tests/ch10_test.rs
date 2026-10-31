//! Unit tests for chapter 10; each mirrors a C++, Kotlin and Python test.

use m4f_ch10::bonds::{
    Flow, coupon_bond, duration, dynamic_hedge, immunise, immunising_weights, investment_value,
    log_return, portfolio_value, price, zero_coupon,
};
use m4f_ch10::term_structure::{
    coupon_bond_price, forward_rate, forward_rates, implied_price, money_market_account,
    prices_from_forward_rates, yield_rate, yield_to_maturity,
};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const EPS: f64 = 1e-9;

/// Sections 10.1.1-10.1.2.
#[test]
fn single_bonds() {
    let z = zero_coupon(100.0, 4.0);
    assert!(
        near(price(&z, 0.06, 0.0), 100.0 * (-0.24_f64).exp(), EPS),
        "zero price"
    );
    assert!(
        near(duration(&z, 0.06, 0.0), 4.0, EPS) && near(duration(&z, 0.06, 1.5), 2.5, EPS),
        "zero duration"
    );
    let b = coupon_bond(100.0, 5.0, 4, 1.0);
    assert!(
        b.len() == 4 && b[3].t == 4.0 && b[3].amount == 105.0 && b[0].amount == 5.0,
        "coupon bond flows"
    );
    let h = 1e-5;
    assert!(
        near(
            (price(&b, 0.06 + h, 0.0) - price(&b, 0.06 - h, 0.0)) / (2.0 * h),
            -duration(&b, 0.06, 0.0) * price(&b, 0.06, 0.0),
            1e-4
        ),
        "dP/dy = -DP"
    );
    assert!(duration(&b, 0.06, 0.0) < 4.0, "D < maturity");
    for horizon in [1.0, 2.5, 4.0] {
        assert!(
            near(
                investment_value(&b, 0.06, horizon),
                price(&b, 0.06, 0.0) * (0.06 * horizon).exp(),
                EPS
            ),
            "unchanged yield grows at y"
        );
        assert!(
            near(log_return(&b, 0.06, 0.06, horizon), 0.06, EPS),
            "log return = y"
        );
    }
    let d = duration(&b, 0.06, 0.0);
    assert!(
        (investment_value(&b, 0.061, d) - investment_value(&b, 0.06, d)).abs() < 1e-2,
        "immunised at t=D"
    );
    assert!(
        investment_value(&b, 0.07, 1.0) < investment_value(&b, 0.06, 1.0),
        "short horizon loses when y rises"
    );
    assert!(
        investment_value(&b, 0.07, 4.0) > investment_value(&b, 0.06, 4.0),
        "long horizon gains when y rises"
    );
}

/// Sections 10.1.3-10.1.4.
#[test]
fn immunisation() {
    let a = coupon_bond(100.0, 4.0, 5, 1.0);
    let b = coupon_bond(100.0, 8.0, 10, 1.0);
    let (w1, w2) = immunising_weights(duration(&a, 0.06, 0.0), duration(&b, 0.06, 0.0), 5.0);
    assert!(
        near(w1 + w2, 1.0, EPS)
            && near(
                w1 * duration(&a, 0.06, 0.0) + w2 * duration(&b, 0.06, 0.0),
                5.0,
                EPS
            ),
        "immunising weights"
    );
    let p = immunise(1000.0, &a, &b, 0.06, 0.0, 5.0);
    assert!(
        near(portfolio_value(&p, &a, &b, 0.06, 0.0), 1000.0, EPS),
        "immunise invests V"
    );
    let target = 1000.0 * (0.06_f64 * 5.0).exp();
    for y in [0.05, 0.07] {
        assert!(
            p.q1 * investment_value(&a, y, 5.0) + p.q2 * investment_value(&b, y, 5.0)
                > target - 1e-9,
            "no shortfall"
        );
    }
    let flat: Vec<(f64, f64)> = (0..11).map(|i| (0.5 * f64::from(i), 0.06)).collect();
    let steps = dynamic_hedge(1000.0, &a, &b, 5.0, &flat, true);
    assert!(
        steps.len() == 11 && near(steps[10].value, 1000.0 * 0.3_f64.exp(), 1e-6),
        "flat path reaches target"
    );
    for st in &steps[..steps.len() - 1] {
        assert!(near(st.duration, 5.0 - st.t, 1e-9), "D = remaining horizon");
    }
    let fixed = dynamic_hedge(1000.0, &a, &b, 5.0, &flat, false);
    assert!(
        near(fixed[fixed.len() - 1].value, 1000.0 * 0.3_f64.exp(), 1e-6),
        "static flat path"
    );
}

/// Section 10.2.
#[test]
fn term_structure() {
    let maturities = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0];
    let prices = [0.978, 0.953, 0.926, 0.898, 0.870, 0.842];
    let forwards = forward_rates(&maturities, &prices);
    assert!(
        near(forwards[0], yield_rate(prices[0], 0.5), EPS),
        "first forward = yield"
    );
    let rebuilt = prices_from_forward_rates(&maturities, &forwards);
    for (r, p) in rebuilt.iter().zip(prices) {
        assert!(near(*r, p, EPS), "prices from forwards");
    }
    assert!(
        near(
            forward_rate(prices[1], 1.0, prices[5], 3.0),
            (forwards[2] + forwards[3] + forwards[4] + forwards[5]) / 4.0,
            EPS
        ),
        "forward average"
    );
    assert!(
        near(
            implied_price(prices[1], prices[3]),
            (-forward_rate(prices[1], 1.0, prices[3], 2.0)).exp(),
            EPS
        ),
        "implied price"
    );
    let account = money_market_account(&maturities, &prices);
    for (a, p) in account.iter().zip(prices) {
        assert!(near(*a, 1.0 / p, EPS), "A = 1/B");
    }
    let p = coupon_bond_price(&[1.0, 2.0, 3.0], &[6.0, 6.0, 106.0], &maturities, &prices);
    assert!(
        near(
            p,
            6.0 * prices[1] + 6.0 * prices[3] + 106.0 * prices[5],
            EPS
        ),
        "coupon bond price"
    );
    let y = yield_to_maturity(p, &[1.0, 2.0, 3.0], &[6.0, 6.0, 106.0]);
    assert!(
        near(
            6.0 * (-y).exp() + 6.0 * (-2.0 * y).exp() + 106.0 * (-3.0 * y).exp(),
            p,
            1e-8
        ),
        "ytm"
    );
    assert!(
        near(
            yield_to_maturity(prices[5] * 100.0, &[3.0], &[100.0]),
            yield_rate(prices[5], 3.0),
            1e-9
        ),
        "ytm of zero"
    );
}

/// The faster `coupon_bond_price`, `yield_to_maturity` and `duration`
/// against the straightforward versions they replaced.
#[test]
fn against_reference() {
    let ref_price = |ft: &[f64], fa: &[f64], mat: &[f64], pr: &[f64]| {
        let mut total = 0.0;
        for (t, a) in ft.iter().zip(fa) {
            if let Some(j) = mat.iter().position(|m| m == t) {
                total += a * pr[j];
            }
        }
        total
    };
    let ref_ytm = |price: f64, ft: &[f64], fa: &[f64]| {
        let (mut lo, mut hi) = (-1.0_f64, 1.0_f64);
        for _ in 0..200 {
            let mid = (lo + hi) / 2.0;
            let mut s = 0.0;
            for (t, a) in ft.iter().zip(fa) {
                s += a * (-mid * t).exp();
            }
            if s > price {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        (lo + hi) / 2.0
    };
    let mat: Vec<f64> = (1..121).map(|k| f64::from(k) / 12.0).collect();
    let pr: Vec<f64> = (1..121)
        .map(|k| (-0.04 * f64::from(k) / 12.0).exp())
        .collect();
    // Sorted, unsorted, off-grid and repeated flow times.
    let times: [&[f64]; 3] = [
        &[1.0 / 12.0, 0.5, 1.0, 10.0],
        &[10.0, 1.0, 0.5, 1.0 / 12.0],
        &[0.3, 1.0, 11.0, 2.0, 2.0],
    ];
    for ft in times {
        let mut fa = vec![5.0; ft.len()];
        let last = fa.len() - 1;
        fa[last] = 105.0;
        assert!(
            coupon_bond_price(ft, &fa, &mat, &pr) == ref_price(ft, &fa, &mat, &pr),
            "coupon_bond_price matches linear search"
        );
    }
    for n in [1, 3, 30, 360] {
        let ft: Vec<f64> = (1..n + 1).map(|i| f64::from(i) / 12.0).collect();
        let fa: Vec<f64> = (1..n + 1)
            .map(|i| if i == n { 100.5 } else { 0.5 })
            .collect();
        // Yields inside the bracket, plus prices whose root lies outside it.
        for price in [50.0, 80.0, 100.0, 101.0, 130.0, 1.0, 1e6] {
            let y = yield_to_maturity(price, &ft, &fa);
            let reference = ref_ytm(price, &ft, &fa);
            assert!(
                (y - reference).abs() <= 1e-13,
                "yield_to_maturity matches bisection"
            );
        }
    }
    let bond = coupon_bond(100.0, 6.0, 10, 0.5);
    for tt in [0.0, 0.7, 4.9] {
        let mut s = 0.0;
        for flow in &bond {
            let Flow { t, amount } = *flow;
            if t > tt {
                s += (t - tt) * amount * (-0.05 * (t - tt)).exp();
            }
        }
        assert!(
            duration(&bond, 0.05, tt) == s / price(&bond, 0.05, tt),
            "duration bit-identical"
        );
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch10::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
