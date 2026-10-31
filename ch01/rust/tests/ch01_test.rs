//! Unit tests for chapter 1; each mirrors a C++, Kotlin and Python test.

use m4f_ch01::forward::{forward_arbitrage, implied_foreign_rate, value0, value1};
use m4f_ch01::fx_arbitrage::{FxMarket, FxQuote};
use m4f_ch01::one_step::{OneStepMarket, Portfolio, State, hedged_purchase_cost};

fn near(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

fn book_fx_market() -> FxMarket {
    FxMarket::new(&[
        FxQuote::new("A", "EUR", "USD", 1.0202, 1.0284),
        FxQuote::new("A", "GBP", "USD", 1.5718, 1.5844),
        FxQuote::new("B", "EUR", "GBP", 0.6324, 0.6401),
        FxQuote::new("B", "USD", "GBP", 0.6299, 0.6375),
    ])
}

#[test]
fn fair_market() {
    let m = OneStepMarket::new(100.0, 110.0, 50.0, 60.0, 40.0, 0.8);
    assert!(
        near(m.r(), 0.1) && near(m.u(), 0.2) && near(m.d(), -0.2),
        "returns"
    );
    assert!(m.is_arbitrage_free(), "no arbitrage");
    assert!(near(m.risk_neutral_prob(), 0.75), "p*");
    assert!(near(m.forward_price(), 55.0), "forward");

    let call = m.replicate(10.0, 0.0);
    assert!(
        near(call.x, 0.5) && near(call.y, -2.0 / 11.0),
        "call replication"
    );
    assert!(near(m.price(10.0, 0.0), 75.0 / 11.0), "call price 6.8182");
    assert!(
        near(m.price(10.0, 0.0), m.risk_neutral_price(10.0, 0.0)),
        "replication == risk-neutral"
    );
    assert!(
        near(m.price(10.0, 0.0) - m.price(0.0, 10.0), 50.0 - 50.0 / 1.1),
        "put-call parity"
    );
}

#[test]
fn arbitrage() {
    for s0 in [55.0, 36.0] {
        let bad = OneStepMarket::new(100.0, 110.0, s0, 60.0, 40.0, 0.8);
        let arb = bad.arbitrage().expect("arbitrage portfolio exists");
        let v_up = bad.value1(&arb, State::Up);
        let v_down = bad.value1(&arb, State::Down);
        assert!(
            near(bad.value0(&arb), 0.0) && v_up >= 0.0 && v_down >= 0.0 && v_up + v_down > 0.0,
            "arbitrage portfolio"
        );
    }
    let fair = OneStepMarket::new(100.0, 110.0, 50.0, 60.0, 40.0, 0.8);
    assert!(
        fair.arbitrage().is_none(),
        "no arbitrage portfolio in fair market"
    );
}

#[test]
#[should_panic(expected = "require 0 < p < 1")]
fn rejects_bad_probability() {
    OneStepMarket::new(100.0, 110.0, 50.0, 60.0, 40.0, 1.0);
}

/// Book: Examples 1.1-1.2, Exercises 1.1-1.2.
#[test]
fn book_examples() {
    let ex = OneStepMarket::new(100.0, 110.0, 50.0, 52.0, 48.0, 0.5);
    let p = Portfolio { x: 20.0, y: 10.0 };
    assert!(
        near(ex.u(), 0.04) && near(ex.d(), -0.04) && !ex.is_arbitrage_free(),
        "Example 1.1"
    );
    assert!(
        near(ex.value0(&p), 2000.0)
            && near(ex.value1(&p, State::Up), 2140.0)
            && near(ex.value1(&p, State::Down), 2060.0)
            && near(ex.portfolio_return(&p, State::Up), 0.07)
            && near(ex.portfolio_return(&p, State::Down), 0.03),
        "Example 1.2"
    );

    let er = OneStepMarket::new(90.0, 100.0, 25.0, 30.0, 20.0, 0.5);
    let q = Portfolio { x: 10.0, y: 15.0 };
    assert!(
        near(er.value0(&q), 1600.0)
            && near(er.value1(&q, State::Up), 1800.0)
            && near(er.value1(&q, State::Down), 1700.0),
        "Exercise 1.1"
    );
    let r = er.replicate(1160.0, 1040.0);
    assert!(
        near(r.x, 12.0) && near(r.y, 8.0) && near(er.value0(&r), 1020.0),
        "Exercise 1.2"
    );
}

/// Exercise 1.3: USD -> GBP (A) -> EUR (B) -> USD (A) gains ~0.594%.
#[test]
fn currency_arbitrage() {
    let market = book_fx_market();
    let cycle = market.best_cycle("USD");
    assert_eq!(cycle.legs.len(), 3);
    assert_eq!(cycle.legs[0].to, "GBP");
    assert_eq!(cycle.legs[1].to, "EUR");
    assert!(
        near(cycle.growth, 1.0202 / (1.5844 * 0.6401)),
        "Exercise 1.3"
    );
}

/// Guards the best-leg table that FxMarket builds for speed. Equal rates go
/// to the first dealer quoted; unquoted pairs and currencies give no leg and
/// an empty cycle.
#[test]
fn fx_leg_lookup() {
    let fx = FxMarket::new(&[
        FxQuote::new("A", "EUR", "USD", 1.10, 1.20),
        FxQuote::new("B", "EUR", "USD", 1.10, 1.15),
        FxQuote::new("C", "GBP", "USD", 1.30, 1.40),
    ]);
    let sell_eur = fx.best_leg("EUR", "USD").expect("EUR -> USD is quoted");
    assert!(
        sell_eur.dealer == "A" && near(sell_eur.rate, 1.10),
        "tie goes to first dealer"
    );
    let buy_eur = fx.best_leg("USD", "EUR").expect("USD -> EUR is quoted");
    assert!(
        buy_eur.dealer == "B" && near(buy_eur.rate, 1.0 / 1.15),
        "best rate wins"
    );
    assert!(
        fx.best_leg("EUR", "GBP").is_none()
            && fx.best_leg("EUR", "JPY").is_none()
            && fx.best_leg("JPY", "EUR").is_none(),
        "missing legs"
    );
    let none = fx.best_cycle("JPY");
    assert!(none.legs.is_empty() && none.growth == 0.0, "unknown start");
}

/// Example 1.4 / Proposition 1.1.
#[test]
fn example_1_4() {
    let m = OneStepMarket::new(100.0, 110.0, 100.0, 125.0, 105.0, 0.5);
    assert!(
        near(m.u(), 0.25)
            && near(m.d(), 0.05)
            && m.is_arbitrage_free()
            && near(m.risk_neutral_prob(), 0.25)
            && near(m.forward_price(), 110.0),
        "Example 1.4"
    );
}

/// Section 1.4 and Exercise 1.4.
#[test]
fn risk_and_return() {
    let rr = OneStepMarket::new(100.0, 110.0, 80.0, 100.0, 60.0, 0.8);
    let has_moments = |p: &Portfolio, mean: f64, sd: f64| {
        let k_up = rr.portfolio_return(p, State::Up);
        let k_down = rr.portfolio_return(p, State::Down);
        near(rr.expect(k_up, k_down), mean) && near(rr.std_dev(k_up, k_down), sd)
    };
    assert!(
        has_moments(&Portfolio { x: 50.0, y: 60.0 }, 0.12, 0.08)
            && has_moments(&Portfolio { x: 0.0, y: 100.0 }, 0.10, 0.0)
            && has_moments(&Portfolio { x: 125.0, y: 0.0 }, 0.15, 0.20),
        "Section 1.4"
    );
    let half = rr.split(10000.0, 0.5);
    assert!(
        near(half.x, 62.5) && near(half.y, 50.0) && has_moments(&half, 0.125, 0.10),
        "Exercise 1.4"
    );
}

/// Section 1.5: Exercises 1.5 and 1.6.
#[test]
fn forwards() {
    let arb = forward_arbitrage(34.0, 100.0, 112.0, 38.60).expect("mispriced forward");
    assert!(
        near(value0(&arb, 34.0, 100.0), 0.0)
            && near(value1(&arb, 30.0, 112.0, 38.60), 0.52)
            && near(value1(&arb, 50.0, 112.0, 38.60), 0.52),
        "Exercise 1.5"
    );
    assert!(
        forward_arbitrage(34.0, 100.0, 112.0, 38.08).is_none(),
        "fair forward has no arbitrage"
    );
    let foreign_rate = implied_foreign_rate(1.6, 1.5, 0.05);
    assert!(
        near(foreign_rate, 0.12) && near(100.0 / (1.0 + foreign_rate), 100.0 / 1.12),
        "Exercise 1.6"
    );
}

/// Section 1.6: Exercises 1.7-1.9.
#[test]
fn options() {
    let o = OneStepMarket::new(100.0, 110.0, 100.0, 120.0, 80.0, 0.5);
    assert!(
        near(o.price(20.0, 0.0), 150.0 / 11.0)
            && near(o.price(30.0, 0.0), 225.0 / 11.0)
            && near(o.price(10.0, 0.0), 75.0 / 11.0),
        "call X=100, Exercise 1.7"
    );
    assert!(
        near(
            OneStepMarket::new(100.0, 105.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0),
            50.0 - 4000.0 / 105.0
        ) && near(
            OneStepMarket::new(100.0, 115.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0),
            50.0 - 4000.0 / 115.0
        ),
        "Exercise 1.8"
    );
    assert!(near(o.price(0.0, 20.0), 50.0 / 11.0), "Exercise 1.9");
}

/// Section 1.7: hedging a future purchase, Exercises 1.11-1.12.
#[test]
fn hedging() {
    let h = OneStepMarket::new(100.0, 110.0, 100.0, 160.0, 40.0, 0.5);
    assert!(near(h.price(60.0, 0.0), 350.0 / 11.0), "C(0)=31.8182");
    assert!(
        near(hedged_purchase_cost(&h, 100.0, 1.0, State::Up), 135.0)
            && near(hedged_purchase_cost(&h, 100.0, 1.0, State::Down), 75.0)
            && near(hedged_purchase_cost(&h, 100.0, 2.0, State::Up), 110.0)
            && near(hedged_purchase_cost(&h, 100.0, 2.0, State::Down), 110.0),
        "hedged cost"
    );
    for p in [0.1, 0.25, 0.5, 0.75, 0.9] {
        let hp = OneStepMarket::new(100.0, 110.0, 100.0, 160.0, 40.0, p);
        let sigma_without = hp.std_dev(160.0, 40.0);
        let sigma_with = hp.std_dev(
            hedged_purchase_cost(&hp, 100.0, 1.0, State::Up),
            hedged_purchase_cost(&hp, 100.0, 1.0, State::Down),
        );
        assert!(near(sigma_with, sigma_without / 2.0), "Exercise 1.12");
    }
}

/// The report matches `expected_output.txt` byte for byte.
#[test]
fn report_matches_expected_output() {
    let mut out = String::new();
    m4f_ch01::report::print_report(&mut out).unwrap();
    assert_eq!(out, include_str!("../../expected_output.txt"));
}
