//! Unit tests for chapter 5; each mirrors a C++, Kotlin and Python test.
#![allow(clippy::needless_range_loop)] // Index loops mirror the C++ tests.

use m4f_ch05::portfolio::{
    Mat, beta, capital_market_line, covariance_matrix, dot, inverse, market_portfolio,
    min_variance_line, min_variance_portfolio, portfolio_mean, portfolio_std, security_market_line,
    solve_symmetric, sum, vec_mat,
};
use m4f_ch05::risk::{
    correlation, covariance, mean, min_variance_weight, std_dev, two_asset_mean, two_asset_std,
};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const EPS: f64 = 1e-9;
const MEANS: [f64; 3] = [0.10, 0.15, 0.20];

fn unit(i: usize) -> Vec<f64> {
    let mut e = vec![0.0; 3];
    e[i] = 1.0;
    e
}

fn market_covariance() -> Mat {
    covariance_matrix(
        &[0.15, 0.25, 0.30],
        &[
            vec![1.0, 0.3, 0.1],
            vec![0.3, 1.0, 0.4],
            vec![0.1, 0.4, 1.0],
        ],
    )
}

/// Section 5.1: scenarios agree with the two-asset formulas.
#[test]
fn scenarios() {
    let p = [0.2, 0.5, 0.3];
    let k1 = [-0.10, 0.10, 0.20];
    let k2 = [0.20, 0.05, -0.05];
    assert!(near(mean(&k1, &p), 0.09, EPS), "mean");
    assert!(near(covariance(&k1, &k2, &p), -0.009, EPS), "covariance");
    let rho = correlation(&k1, &k2, &p);
    for w in [0.0, 0.3, 0.5, 1.2] {
        let k_v: Vec<f64> = (0..3).map(|i| w * k1[i] + (1.0 - w) * k2[i]).collect();
        assert!(
            near(
                std_dev(&k_v, &p),
                two_asset_std(w, std_dev(&k1, &p), std_dev(&k2, &p), rho),
                EPS
            ),
            "two-asset sigma"
        );
        assert!(
            near(
                mean(&k_v, &p),
                two_asset_mean(w, mean(&k1, &p), mean(&k2, &p)),
                EPS
            ),
            "two-asset mu"
        );
    }
}

/// Section 5.2.1: the minimum variance weight is a local minimum.
#[test]
fn min_variance_weight_is_minimal() {
    for rho in [-0.9, -0.5, 0.0, 0.3] {
        let w = min_variance_weight(0.10, 0.20, rho);
        let s = two_asset_std(w, 0.10, 0.20, rho);
        assert!(
            s < two_asset_std(w - 0.01, 0.10, 0.20, rho)
                && s < two_asset_std(w + 0.01, 0.10, 0.20, rho),
            "min variance"
        );
    }
    assert!(
        near(two_asset_std(2.0 / 3.0, 0.10, 0.20, -1.0), 0.0, 1e-7),
        "riskless at rho=-1"
    );
}

#[test]
fn inverse_times_matrix_is_identity() {
    let c = market_covariance();
    let c_inv = inverse(&c);
    for i in 0..3 {
        let row = vec_mat(&unit(i), &c);
        for j in 0..3 {
            let column = [c_inv[0][j], c_inv[1][j], c_inv[2][j]];
            assert!(
                near(dot(&row, &column), if i == j { 1.0 } else { 0.0 }, EPS),
                "C C^-1 = I"
            );
        }
    }
}

/// The Cholesky solve against the explicit inverse, on a 40-asset positive
/// definite matrix and on an indefinite symmetric matrix (which takes the
/// fallback path).
#[test]
fn solve_symmetric_matches_inverse() {
    const N: usize = 40;
    // F F^T + D: three factors plus idiosyncratic variance.
    let spd: Mat = (0..N)
        .map(|i| {
            (0..N)
                .map(|j| {
                    let mut sum = if i == j { 0.02 + 0.001 * i as f64 } else { 0.0 };
                    for f in 1..4 {
                        let f = f64::from(f);
                        sum += 0.01 * (f * (i as f64 + 1.0)).cos() * (f * (j as f64 + 1.0)).cos();
                    }
                    sum
                })
                .collect()
        })
        .collect();
    let indefinite = vec![
        vec![1.0, 2.0, 0.0],
        vec![2.0, 1.0, 0.0],
        vec![0.0, 0.0, 3.0],
    ];
    for c in [spd, indefinite] {
        let b: Vec<f64> = (0..c.len()).map(|i| 1.0 + 0.1 * i as f64).collect();
        let expected = vec_mat(&b, &inverse(&c));
        let x = solve_symmetric(&c, vec![b]).swap_remove(0);
        let worst = x
            .iter()
            .zip(&expected)
            .map(|(x, e)| (x - e).abs() / e.abs().max(1.0))
            .fold(0.0, f64::max);
        assert!(worst < 1e-9, "solve_symmetric matches the inverse");
    }
}

/// Section 5.3.2.
#[test]
fn efficient_frontier() {
    let c = market_covariance();
    let w_min = min_variance_portfolio(&c);
    assert!(near(sum(&w_min), 1.0, EPS), "weights sum to 1");
    assert!(
        near(
            min_variance_line(&MEANS, &c, portfolio_mean(&w_min, &MEANS))[0],
            w_min[0],
            EPS
        ),
        "MVP on the line"
    );
    for mu in [0.12, 0.15, 0.25] {
        let w = min_variance_line(&MEANS, &c, mu);
        assert!(
            near(sum(&w), 1.0, EPS) && near(portfolio_mean(&w, &MEANS), mu, EPS),
            "line"
        );
        assert!(
            portfolio_std(&w, &c) >= portfolio_std(&w_min, &c),
            "above MVP"
        );
        assert!(
            portfolio_std(&w, &c) < portfolio_std(&[w[0] + 0.01, w[1] - 0.02, w[2] + 0.01], &c),
            "minimal at mu"
        );
    }
}

/// Section 5.4.
#[test]
fn capm() {
    let c = market_covariance();
    let w_market = market_portfolio(&MEANS, &c, 0.05);
    let mu_m = portfolio_mean(&w_market, &MEANS);
    let sigma_m = portfolio_std(&w_market, &c);
    assert!(
        near(sum(&w_market), 1.0, EPS) && near(beta(&w_market, &w_market, &c), 1.0, EPS),
        "market beta"
    );
    assert!(
        near(capital_market_line(sigma_m, 0.05, mu_m, sigma_m), mu_m, EPS),
        "CML through M"
    );
    assert!(
        near(
            portfolio_std(&min_variance_line(&MEANS, &c, mu_m), &c),
            sigma_m,
            EPS
        ),
        "M on frontier"
    );
    for i in 0..3 {
        assert!(
            near(
                security_market_line(beta(&unit(i), &w_market, &c), 0.05, mu_m),
                MEANS[i],
                EPS
            ),
            "SML"
        );
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch05::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
