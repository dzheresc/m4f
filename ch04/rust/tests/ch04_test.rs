//! Unit tests for chapter 4; each mirrors a C++, Kotlin and Python test.

use m4f_ch04::no_arbitrage::{
    Rule, arbitrage_strategy, expected_discounted_value, is_arbitrage_free, one_step_values,
    risk_neutral_probs,
};
use m4f_ch04::strategies::{discounted_gains, gains, self_financing, self_financing_multi};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const EPS: f64 = 1e-9;

fn count_ups(moves: &[i32]) -> i32 {
    moves.iter().sum()
}

/// Section 4.1.1.
#[test]
fn self_financing_strategy() {
    let s = [100.0, 110.0, 104.5, 114.95];
    let a: Vec<f64> = (0..4).map(|n| 100.0 * 1.03f64.powf(f64::from(n))).collect();
    let x = [5.0, 8.0, 3.0];
    let path = self_financing(1000.0, &s, &a, &x);
    let (y, values) = (&path.y, &path.values);
    assert!(near(values[3], 1079.2915, 1e-4), "V(3)");
    for n in 1..3 {
        assert!(
            near(x[n] * s[n] + y[n] * a[n], values[n], EPS),
            "rebalancing costs V(n)"
        );
    }
    assert!(
        near(gains(&s, &a, &x, y), values[3] - values[0], EPS),
        "gains"
    );
    assert!(
        near(
            discounted_gains(&s, &a, &x),
            values[3] / a[3] - values[0] / a[0],
            EPS
        ),
        "discounted gains"
    );
}

/// Section 4.2.
#[test]
fn dividends() {
    let s = [vec![50.0, 55.0, 52.0, 58.0], vec![20.0, 19.0, 21.0, 22.0]];
    let a: Vec<f64> = (0..4).map(|n| 1.05f64.powf(f64::from(n))).collect();
    let x = [vec![10.0, 12.0, 8.0], vec![20.0, 15.0, 25.0]];
    let dividends = [vec![0.0, 0.0, 0.0, 0.0], vec![0.0, 0.0, 2.0, 0.0]];
    assert!(
        near(
            self_financing_multi(1000.0, &s, &a, &x, Some(&dividends)).values[3]
                - self_financing_multi(1000.0, &s, &a, &x, None).values[3],
            2.0 * 15.0 * 1.05,
            EPS
        ),
        "dividend"
    );
}

/// Section 4.1.2.
#[test]
fn arbitrage() {
    let returns = [0.10, -0.05];
    assert!(
        is_arbitrage_free(&returns, 0.03)
            && arbitrage_strategy(&returns, 0.03, 100.0, 100.0).is_none(),
        "no arbitrage"
    );
    for r in [0.10, 0.12, -0.05, -0.08] {
        let (x, y) = arbitrage_strategy(&returns, r, 100.0, 100.0).expect("arbitrage");
        assert!(near(x * 100.0 + y * 100.0, 0.0, EPS), "V(0) = 0");
        let v1 = one_step_values(x, y, &returns, r, 100.0, 100.0);
        assert!(
            v1[0].min(v1[1]) >= -1e-12 && v1[0].max(v1[1]) > 0.0,
            "V(1) >= 0, > 0 somewhere"
        );
    }
    assert!(
        near(
            risk_neutral_probs(&returns, 0.03).unwrap()[0],
            8.0 / 15.0,
            EPS
        ),
        "binomial p*"
    );
}

/// Section 4.1.4.
#[test]
fn fundamental_theorem() {
    let markets: [&[f64]; 3] = [&[0.10, -0.05], &[-0.10, 0.0, 0.10], &[0.3, -0.2, 0.05, 0.1]];
    for returns in markets {
        for r in [-0.3, -0.09, 0.0, 0.04, 0.09, 0.5] {
            let q = risk_neutral_probs(returns, r);
            assert_eq!(q.is_some(), is_arbitrage_free(returns, r), "FTAP");
            let Some(q) = q else { continue };
            let sum: f64 = q.iter().sum();
            let mean: f64 = q.iter().zip(returns).map(|(qi, k)| qi * k).sum();
            let smallest = q.iter().copied().fold(1.0, f64::min);
            assert!(
                near(sum, 1.0, EPS) && near(mean, r, EPS) && smallest > 0.0,
                "risk-neutral q"
            );
        }
    }
}

/// Section 4.1.3: martingale property of discounted self-financing values.
#[test]
fn martingale() {
    let q = 8.0 / 15.0;
    let contrarian = |_: i32, moves: &[i32]| 1.0 + f64::from(moves.len() as i32 - count_ups(moves));
    let momentum = |_: i32, moves: &[i32]| 3.0 * f64::from(count_ups(moves)) - 2.0;
    let by_time = |n: i32, _: &[i32]| f64::from(n);
    let rules: [Rule<'_>; 3] = [&contrarian, &momentum, &by_time];
    for rule in rules {
        for v0 in [0.0, 100.0] {
            assert!(
                near(
                    expected_discounted_value(100.0, 0.10, -0.05, 0.03, 4, v0, rule, q),
                    v0,
                    EPS
                ),
                "E*(V~(N)) = V(0)"
            );
        }
    }
    // The depth-first walk against a direct enumeration of all paths, under
    // a real-world p, for each rule.
    const STEPS: usize = 6;
    for rule in rules {
        let mut expected = 0.0;
        for mask in 0..(1 << STEPS) {
            let moves: Vec<i32> = (0..STEPS).map(|i| (mask >> i) & 1).collect();
            let mut s = vec![100.0];
            let mut a = vec![1.0];
            let mut x = Vec::new();
            let mut weight = 1.0;
            for n in 1..=STEPS {
                let up = moves[n - 1] == 1;
                s.push(s[n - 1] * if up { 1.10 } else { 0.95 });
                a.push(a[n - 1] * 1.03);
                x.push(rule(n as i32, &moves[..n - 1]));
                weight *= if up { 0.6 } else { 0.4 };
            }
            expected += weight * self_financing(100.0, &s, &a, &x).values[STEPS] / a[STEPS];
        }
        assert!(
            near(
                expected_discounted_value(100.0, 0.10, -0.05, 0.03, STEPS as i32, 100.0, rule, 0.6),
                expected,
                EPS
            ),
            "E(V~(N)) matches direct enumeration"
        );
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch04::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
