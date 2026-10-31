//! Unit tests for chapter 3; each mirrors a C++, Kotlin and Python test.

use m4f_ch03::binomial_tree::{BinomialTree, binomial_prob};
use m4f_ch03::other_models::{
    TrinomialModel, binomial_expected_price, binomial_prob_above, lognormal_expected_price,
    lognormal_prob_above, norm_cdf,
};
use m4f_ch03::returns::{
    compound_return, expectation, log_return, log_returns, one_step_return, returns, variance,
};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const EPS: f64 = 1e-9;

#[test]
fn report_values() {
    let tree = BinomialTree {
        s0: 100.0,
        u: 0.10,
        d: -0.05,
        r: 0.03,
    };
    assert!(near(tree.expected_price(3, 0.6), 112.4864, EPS), "E(S(3))");
    assert!(near(tree.risk_neutral_prob(), 8.0 / 15.0, EPS), "p*");
    let model = TrinomialModel {
        u: 0.10,
        m: 0.0,
        d: -0.10,
        r: 0.02,
    };
    assert!(near(model.max_middle_prob(), 0.8, EPS), "p_m bound");
    assert!(
        near(
            lognormal_expected_price(100.0, 0.05, 0.2, 1.0),
            100.0 * 0.07f64.exp(),
            EPS
        ),
        "lognormal mean"
    );
}

/// Section 3.1.
#[test]
fn returns_and_moments() {
    let path = [45.0, 48.0, 47.5, 50.0];
    assert!(
        near(
            compound_return(&returns(&path)),
            one_step_return(45.0, 50.0, 0.0),
            EPS
        ),
        "compound return"
    );
    let k = log_returns(&path);
    assert!(
        near(k[0] + k[1] + k[2], log_return(45.0, 50.0), EPS),
        "log returns add"
    );
    assert!(
        near(expectation(&[1.0, 3.0], &[0.5, 0.5]), 2.0, EPS),
        "expectation"
    );
    assert!(
        near(variance(&[1.0, 3.0], &[0.5, 0.5]), 1.0, EPS),
        "variance"
    );
}

/// Section 3.2.
#[test]
fn binomial_tree() {
    let total: f64 = (0..=5).map(|k| binomial_prob(5, k, 0.3)).sum();
    assert!(near(total, 1.0, EPS), "binomial probabilities sum to 1");
    // Large n: C(2000, 1000) alone overflows a double.
    let big_total: f64 = (0..=2000).map(|k| binomial_prob(2000, k, 0.6)).sum();
    assert!(near(big_total, 1.0, EPS), "large-n probabilities sum to 1");
    assert!(
        near(binomial_prob(2000, 1000, 0.5), 0.017839011, 1e-9),
        "C(2000,1000)/2^2000"
    );
    let long_tree = BinomialTree {
        s0: 100.0,
        u: 0.02,
        d: -0.01,
        r: 0.005,
    };
    assert!(
        near(
            long_tree.expected_price(2000, 0.6)
                / (100.0 * (1.0 + 0.6 * 0.02 + 0.4 * -0.01_f64).powf(2000.0)),
            1.0,
            EPS
        ),
        "E(S(2000)) closed form"
    );
    let tree = BinomialTree {
        s0: 60.0,
        u: 0.2,
        d: -0.1,
        r: 0.05,
    };
    let q = tree.risk_neutral_prob();
    assert!(tree.is_arbitrage_free(), "d < r < u");
    assert!(near(tree.expected_return(q), 0.05, EPS), "E*(K) = r");
    for n in 0..4 {
        let nf = f64::from(n);
        assert!(
            near(tree.expected_price(n, q), 60.0 * 1.05f64.powf(nf), EPS),
            "E*(S(n))"
        );
        assert!(
            near(
                tree.expected_price(n, 0.4),
                60.0 * (1.0 + tree.expected_return(0.4)).powf(nf),
                EPS
            ),
            "E(S(n))"
        );
        for k in 0..=n {
            assert!(
                near(
                    tree.conditional_discounted(n, k),
                    tree.discounted_price(n, k),
                    EPS
                ),
                "martingale"
            );
        }
    }
    assert!(
        !BinomialTree {
            s0: 60.0,
            u: 0.2,
            d: 0.1,
            r: 0.05
        }
        .is_arbitrage_free(),
        "arbitrage when r < d"
    );
}

/// Section 3.3.1.
#[test]
fn trinomial() {
    let model = TrinomialModel {
        u: 0.10,
        m: 0.0,
        d: -0.10,
        r: 0.02,
    };
    for p_mid in [0.1, 0.5, 0.79] {
        let p = model.risk_neutral_probs(p_mid);
        assert!(near(p.p_up + p.p_mid + p.p_down, 1.0, EPS), "trinomial sum");
        assert!(
            near(p.p_up * 0.10 + p.p_down * -0.10, 0.02, EPS),
            "trinomial E*(K) = r"
        );
        assert!(p.p_up > 0.0 && p.p_down > 0.0, "trinomial positive");
    }
    assert!(
        model.risk_neutral_probs(0.81).p_down < 0.0,
        "p_m above bound"
    );
}

/// Section 3.3.2.
#[test]
fn continuous_limit() {
    assert!(near(norm_cdf(0.0), 0.5, 1e-7), "Phi(0)");
    assert!(near(norm_cdf(1.96), 0.9750021, 1e-6), "Phi(1.96)");
    assert!(
        near(norm_cdf(-1.0) + norm_cdf(1.0), 1.0, EPS),
        "Phi symmetry"
    );
    assert!(
        near(
            binomial_expected_price(100.0, 0.05, 0.2, 1.0, 100000),
            lognormal_expected_price(100.0, 0.05, 0.2, 1.0),
            1e-4
        ),
        "E(S(T)) limit"
    );
    assert!(
        near(
            binomial_prob_above(100.0, 0.05, 0.2, 1.0, 20000, 110.0),
            lognormal_prob_above(100.0, 0.05, 0.2, 1.0, 110.0),
            1e-2
        ),
        "P(S(T)>K) limit"
    );
    assert!(
        near(binomial_prob_above(100.0, 0.0, 0.2, 1.0, 7, 1e-9), 1.0, EPS),
        "P(S(T)>0) = 1"
    );
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch03::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
