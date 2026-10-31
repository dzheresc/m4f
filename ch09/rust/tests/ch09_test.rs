//! Unit tests for chapter 9; each mirrors a C++, Kotlin and Python test.

use m4f_ch09::black_scholes::{Greek, bs_greeks, bs_price, norm_cdf, norm_inv, norm_pdf};
use m4f_ch09::hedging::{
    Position, delta_greek_hedge, delta_hedge, portfolio_value, rebalance_path, total_greek,
};
use m4f_ch09::risk::{
    Lognormal, all_in_calls, all_in_stock, breakeven, bull_spread, cdf, expected_call_payoff,
    expected_put_payoff, expected_return, expected_stock, initial_cost, prob_loss,
    put_hedged_holding, quantile, terminal_value, unhedged_holding, value_at_risk,
};

fn near(a: f64, b: f64, eps: f64) -> bool {
    (a - b).abs() < eps
}

const S: f64 = 100.0;
const STRIKE: f64 = 100.0;
const RATE: f64 = 0.05;
const SIGMA: f64 = 0.2;
const MATURITY: f64 = 0.5;

#[test]
fn normal() {
    assert!(near(norm_cdf(0.0), 0.5, 1e-7), "N(0)");
    assert!(near(norm_cdf(1.959963984540054), 0.975, 1e-9), "N(1.96)");
    assert!(
        near(norm_inv(0.975), 1.959963984540054, 1e-9),
        "N^-1(0.975)"
    );
    assert!(
        near(
            norm_pdf(0.0),
            1.0 / (2.0 * std::f64::consts::PI).sqrt(),
            1e-12
        ),
        "pdf(0)"
    );
}

/// `norm_inv` against the bisection it replaced, across the center, both
/// tails and the region boundaries of the rational approximation. Near
/// p = 1 the inverse is only defined to within one ulp of p divided by the
/// density, so the tolerance includes that term.
#[test]
fn norm_inv_against_bisection() {
    let bisect = |p: f64| {
        let (mut lo, mut hi) = (-40.0_f64, 40.0_f64);
        for _ in 0..200 {
            let mid = (lo + hi) / 2.0;
            if norm_cdf(mid) < p {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        (lo + hi) / 2.0
    };
    for p in [
        1e-100, 1e-20, 1e-10, 1e-4, 0.001, 0.01, 0.02425, 0.0243, 0.1, 0.3, 0.5, 0.7, 0.95, 0.975,
        0.97575, 0.99, 0.999, 0.999999,
    ] {
        let x = norm_inv(p);
        let reference = bisect(p);
        let tol = 1e-13 + 4e-16 / norm_pdf(reference);
        assert!((x - reference).abs() <= tol, "norm_inv vs bisection at {p}");
        assert!(
            (norm_cdf(x) - p).abs() <= 1e-14 * p.max(1e-2),
            "N(norm_inv(p)) == p at {p}"
        );
    }
    assert!(
        norm_inv(0.0) == -40.0 && norm_inv(1.0) == 40.0,
        "norm_inv ends"
    );
    // Beyond norm_cdf's range (|x| > 37) the result stays finite and ordered.
    assert!(
        norm_inv(1e-300) < -37.0 && norm_inv(1e-300) > -40.0,
        "norm_inv(1e-300)"
    );
}

/// Greeks against finite differences.
#[test]
fn greeks() {
    let h = 1e-4;
    for is_call in [true, false] {
        let g = bs_greeks(S, STRIKE, RATE, SIGMA, MATURITY, is_call);
        let p = |s, x, r, sigma, maturity| bs_price(s, x, r, sigma, maturity, is_call);
        assert!(
            near(g.price, p(S, STRIKE, RATE, SIGMA, MATURITY), 1e-7),
            "price"
        );
        assert!(
            near(
                g.delta,
                (p(S + h, STRIKE, RATE, SIGMA, MATURITY) - p(S - h, STRIKE, RATE, SIGMA, MATURITY))
                    / (2.0 * h),
                1e-6
            ),
            "delta"
        );
        assert!(
            near(
                g.gamma,
                (p(S + h, STRIKE, RATE, SIGMA, MATURITY)
                    - 2.0 * p(S, STRIKE, RATE, SIGMA, MATURITY)
                    + p(S - h, STRIKE, RATE, SIGMA, MATURITY))
                    / (h * h),
                1e-4
            ),
            "gamma"
        );
        assert!(
            near(
                g.theta,
                -(p(S, STRIKE, RATE, SIGMA, MATURITY + h)
                    - p(S, STRIKE, RATE, SIGMA, MATURITY - h))
                    / (2.0 * h),
                1e-5
            ),
            "theta"
        );
        assert!(
            near(
                g.vega,
                (p(S, STRIKE, RATE, SIGMA + h, MATURITY) - p(S, STRIKE, RATE, SIGMA - h, MATURITY))
                    / (2.0 * h),
                1e-5
            ),
            "vega"
        );
        assert!(
            near(
                g.rho,
                (p(S, STRIKE, RATE + h, SIGMA, MATURITY) - p(S, STRIKE, RATE - h, SIGMA, MATURITY))
                    / (2.0 * h),
                1e-5
            ),
            "rho"
        );
    }
}

/// Hedges are neutral and self-financing.
#[test]
fn hedges() {
    let written = [Position {
        qty: -1000.0,
        strike: STRIKE,
        expiry: MATURITY,
        is_call: true,
    }];
    let extra = Position {
        qty: 0.0,
        strike: 110.0,
        expiry: MATURITY,
        is_call: true,
    };
    let h_delta = delta_hedge(&written, S, RATE, SIGMA);
    let h_gamma = delta_greek_hedge(&written, &extra, S, RATE, SIGMA, Greek::Gamma);
    let h_vega = delta_greek_hedge(&written, &extra, S, RATE, SIGMA, Greek::Vega);
    for hedge in [&h_delta, &h_gamma, &h_vega] {
        assert!(
            near(portfolio_value(hedge, S, 0.0, RATE, SIGMA), 0.0, 1e-8),
            "hedge value 0 at t=0"
        );
        assert!(
            near(
                total_greek(&hedge.options, S, RATE, SIGMA, Greek::Delta) + hedge.shares,
                0.0,
                1e-9
            ),
            "delta neutral"
        );
    }
    assert!(
        near(
            total_greek(&h_gamma.options, S, RATE, SIGMA, Greek::Gamma),
            0.0,
            1e-9
        ),
        "gamma neutral"
    );
    assert!(
        near(
            total_greek(&h_vega.options, S, RATE, SIGMA, Greek::Vega),
            0.0,
            1e-6
        ),
        "vega neutral"
    );
    assert!(
        (portfolio_value(&h_delta, S + 1.0, 0.0, RATE, SIGMA)
            - total_greek(&written, S, RATE, SIGMA, Greek::Gamma) / 2.0)
            .abs()
            < 0.5,
        "delta hedge error ~ gamma/2"
    );
    // Rebalancing is self-financing.
    let steps = rebalance_path(&written, &[100.0, 103.0, 101.0], 1.0 / 52.0, RATE, SIGMA);
    assert!(
        steps.len() == 3 && near(steps[0].value, 0.0, 1e-8),
        "path start"
    );
    assert!(
        near(
            steps[1].money,
            steps[0].money * (RATE / 52.0).exp() - (steps[1].shares - steps[0].shares) * steps[1].s,
            1e-6
        ),
        "path self-financing"
    );
}

#[test]
fn lognormal() {
    let model = Lognormal {
        s0: 100.0,
        m: 0.08,
        sigma: 0.3,
        maturity: 0.5,
    };
    assert!(
        near(cdf(&model, quantile(&model, 0.3)), 0.3, 1e-9),
        "quantile/cdf"
    );
    assert!(
        near(
            expected_call_payoff(&model, 90.0) - expected_put_payoff(&model, 90.0),
            expected_stock(&model) - 90.0,
            1e-9
        ),
        "parity"
    );
    let n = 20000;
    let (lo, hi) = (-8.0, 8.0);
    let mut total = 0.0;
    for i in 0..n {
        let z = lo + (f64::from(i) + 0.5) * (hi - lo) / f64::from(n);
        let s = 100.0 * (0.08 * 0.5 + 0.3 * 0.5_f64.sqrt() * z).exp();
        total += (s - 90.0).max(0.0) * norm_pdf(z) * (hi - lo) / f64::from(n);
    }
    assert!(
        near(total, expected_call_payoff(&model, 90.0), 1e-6),
        "expected call payoff by quadrature"
    );
}

/// VaR and strategies.
#[test]
fn strategies() {
    let model = Lognormal {
        s0: 100.0,
        m: 0.08,
        sigma: 0.3,
        maturity: 0.5,
    };
    let stock = unhedged_holding(1.0);
    assert!(
        near(
            value_at_risk(&stock, &model, RATE, 0.3, 0.95),
            100.0 - quantile(&model, 0.05),
            1e-7
        ),
        "VaR stock"
    );
    let hedged = put_hedged_holding("p", 1.0, 95.0, 1.0);
    assert!(
        near(
            value_at_risk(&hedged, &model, RATE, 0.3, 0.95),
            bs_price(100.0, 95.0, RATE, 0.3, 0.5, false) + 5.0,
            1e-7
        ),
        "VaR hedged"
    );
    assert!(
        expected_return(&hedged, &model, RATE, 0.3) < expected_return(&stock, &model, RATE, 0.3),
        "hedging costs return"
    );
    let v0 = 10000.0;
    for st in [
        all_in_stock(v0, 100.0),
        all_in_calls("c", v0, 100.0, 100.0, RATE, 0.25, 0.5),
        bull_spread("b", v0, 100.0, 100.0, 115.0, RATE, 0.25, 0.5),
    ] {
        assert!(
            near(initial_cost(&st, 100.0, RATE, 0.25, 0.5), v0, 1e-7),
            "initial cost = V0"
        );
        let b = breakeven(&st, &model, RATE, 0.25);
        assert!(
            near(terminal_value(&st, b, RATE, 0.5), v0, 1e-6),
            "breakeven"
        );
        assert!(
            near(prob_loss(&st, &model, RATE, 0.25), cdf(&model, b), 1e-7),
            "P(loss)"
        );
    }
    assert!(
        near(
            breakeven(
                &all_in_calls("c", v0, 100.0, 100.0, RATE, 0.25, 0.5),
                &model,
                RATE,
                0.25
            ),
            100.0 + bs_price(100.0, 100.0, RATE, 0.25, 0.5, true),
            1e-6
        ),
        "call breakeven = X + C"
    );
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch09::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
