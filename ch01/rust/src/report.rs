//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::forward::{
    forward_arbitrage, forward_price, implied_foreign_rate, long_forward_payoff,
    short_forward_payoff, value0, value1,
};
use crate::fx_arbitrage::{FxMarket, FxQuote};
use crate::one_step::{
    OneStepMarket, Portfolio, State, call_payoff, hedged_purchase_cost, put_payoff,
};

/// Four decimals, round-half-even on the exact double, like `{:.4f}` in
/// C++ `std::format`. Adding 0.0 turns -0.0 into 0.0 so that no "-0.0000"
/// appears.
struct F4(f64);

impl Display for F4 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:.4}", self.0 + 0.0)
    }
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    print_returns_and_portfolios(os)?;
    print_currency_arbitrage(os)?;
    print_binomial_tree(os)?;
    print_risk_and_return(os)?;
    print_forwards(os)?;
    print_options(os)?;
    print_hedging(os)
}

fn print_portfolio_line(
    os: &mut impl Write,
    market: &OneStepMarket,
    name: &str,
    portfolio: &Portfolio,
) -> fmt::Result {
    let k_up = market.portfolio_return(portfolio, State::Up);
    let k_down = market.portfolio_return(portfolio, State::Down);
    writeln!(
        os,
        "{name}: x={} y={} V(0)={} V(1)={}|{} K_V={}|{}",
        F4(portfolio.x),
        F4(portfolio.y),
        F4(market.value0(portfolio)),
        F4(market.value1(portfolio, State::Up)),
        F4(market.value1(portfolio, State::Down)),
        F4(k_up),
        F4(k_down)
    )
}

fn print_risk_line(
    os: &mut impl Write,
    market: &OneStepMarket,
    name: &str,
    portfolio: &Portfolio,
) -> fmt::Result {
    let k_up = market.portfolio_return(portfolio, State::Up);
    let k_down = market.portfolio_return(portfolio, State::Down);
    writeln!(
        os,
        "{name}: x={} y={} V(1)={}|{} K_V={}|{} E(K_V)={} sigma_V={}",
        F4(portfolio.x),
        F4(portfolio.y),
        F4(market.value1(portfolio, State::Up)),
        F4(market.value1(portfolio, State::Down)),
        F4(k_up),
        F4(k_down),
        F4(market.expect(k_up, k_down)),
        F4(market.std_dev(k_up, k_down))
    )
}

fn print_option_line(
    os: &mut impl Write,
    market: &OneStepMarket,
    name: &str,
    d_up: f64,
    d_down: f64,
) -> fmt::Result {
    let replica = market.replicate(d_up, d_down);
    writeln!(
        os,
        "{name}: payoff {}|{} replicate x={} y={} price={}",
        F4(d_up),
        F4(d_down),
        F4(replica.x),
        F4(replica.y),
        F4(market.price(d_up, d_down))
    )
}

fn print_arbitrage_lines(os: &mut impl Write, market: &OneStepMarket) -> fmt::Result {
    writeln!(
        os,
        "d < r < u: {} < {} < {} -> arbitrage-free: {}",
        F4(market.d()),
        F4(market.r()),
        F4(market.u()),
        if market.is_arbitrage_free() {
            "yes"
        } else {
            "no"
        }
    )?;
    if let Some(arb) = market.arbitrage() {
        writeln!(
            os,
            "Arbitrage: x={} y={} V(0)={} V(1)={}|{}",
            F4(arb.x),
            F4(arb.y),
            F4(market.value0(&arb)),
            F4(market.value1(&arb, State::Up)),
            F4(market.value1(&arb, State::Down))
        )?;
    }
    Ok(())
}

/// Examples 1.1-1.2, Exercises 1.1-1.2 and the no-arbitrage check.
fn print_returns_and_portfolios(os: &mut impl Write) -> fmt::Result {
    // The book leaves p unspecified (0 < p < 1); it does not affect anything
    // printed here.
    let ex = OneStepMarket::new(100.0, 110.0, 50.0, 52.0, 48.0, 0.5);

    writeln!(os, "== Example 1.1: returns ==")?;
    writeln!(
        os,
        "A(0)={} A(1)={} S(0)={} S(1)={}|{}",
        F4(ex.a0()),
        F4(ex.a1()),
        F4(ex.s0()),
        F4(ex.s1(State::Up)),
        F4(ex.s1(State::Down))
    )?;
    writeln!(os, "K_A={} K_S={}|{}", F4(ex.r()), F4(ex.u()), F4(ex.d()))?;

    writeln!(os, "\n== Example 1.2: portfolio value and return ==")?;
    print_portfolio_line(os, &ex, "Portfolio", &Portfolio { x: 20.0, y: 10.0 })?;

    // Exercises 1.1-1.2 (again p is left unspecified).
    let exr = OneStepMarket::new(90.0, 100.0, 25.0, 30.0, 20.0, 0.5);
    writeln!(
        os,
        "\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 =="
    )?;
    print_portfolio_line(os, &exr, "Portfolio", &Portfolio { x: 10.0, y: 15.0 })?;

    writeln!(os, "\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==")?;
    print_portfolio_line(os, &exr, "Solution", &exr.replicate(1160.0, 1040.0))?;

    writeln!(os, "\n== No-arbitrage check on the Example 1.1 market ==")?;
    print_arbitrage_lines(os, &ex)
}

/// Exercise 1.3.
fn print_currency_arbitrage(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== Exercise 1.3: currency dealers A (New York) and B (London) =="
    )?;
    let fx = FxMarket::new(&[
        FxQuote::new("A", "EUR", "USD", 1.0202, 1.0284),
        FxQuote::new("A", "GBP", "USD", 1.5718, 1.5844),
        FxQuote::new("B", "EUR", "GBP", 0.6324, 0.6401),
        FxQuote::new("B", "USD", "GBP", 0.6299, 0.6375),
    ]);
    let cycle = fx.best_cycle("USD");
    write!(os, "Best round trip: USD")?;
    for leg in &cycle.legs {
        write!(os, " -> {}", leg.to)?;
    }
    writeln!(os)?;
    for leg in &cycle.legs {
        writeln!(
            os,
            "  {} -> {} with dealer {}: rate {:.6}",
            leg.from, leg.to, leg.dealer, leg.rate
        )?;
    }
    writeln!(
        os,
        "1 USD -> {:.6} USD: {}",
        cycle.growth,
        if cycle.growth > 1.0 {
            "arbitrage (borrow USD, run the cycle, repay)"
        } else {
            "no arbitrage"
        }
    )
}

/// Example 1.4 / Figure 1.1 and the proof of Proposition 1.1.
fn print_binomial_tree(os: &mut impl Write) -> fmt::Result {
    // p is left unspecified by the book.
    let market = OneStepMarket::new(100.0, 110.0, 100.0, 125.0, 105.0, 0.5);
    let s_up = market.s1(State::Up);
    let s_down = market.s1(State::Down);
    writeln!(
        os,
        "\n== Example 1.4: one-step binomial tree (Figure 1.1) =="
    )?;
    writeln!(
        os,
        "A(0)={} A(1)={} S(0)={} S(1)={}|{}",
        F4(market.a0()),
        F4(market.a1()),
        F4(market.s0()),
        F4(s_up),
        F4(s_down)
    )?;
    writeln!(
        os,
        "K_A={} K_S={}|{}",
        F4(market.r()),
        F4(market.u()),
        F4(market.d())
    )?;
    writeln!(
        os,
        "Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: {} < {} < {} -> {}",
        F4(s_down),
        F4(market.a1()),
        F4(s_up),
        if s_down < market.a1() && market.a1() < s_up {
            "holds"
        } else {
            "fails"
        }
    )?;
    // Proof of Proposition 1.1: the two violating cases (illustrative S(1)
    // values).
    for (up, down, label) in [(130.0, 110.0, "A(1) <= S^d"), (110.0, 90.0, "A(1) >= S^u")] {
        let bad = OneStepMarket::new(100.0, 110.0, 100.0, up, down, 0.5);
        let arb = bad.arbitrage().expect("the market admits arbitrage");
        writeln!(
            os,
            "  proof, case {label} (S(1)={}|{}): x={} y={} V(0)={} V(1)={}|{}",
            F4(up),
            F4(down),
            F4(arb.x),
            F4(arb.y),
            F4(bad.value0(&arb)),
            F4(bad.value1(&arb, State::Up)),
            F4(bad.value1(&arb, State::Down))
        )?;
    }
    print_arbitrage_lines(os, &market)
}

/// Section 1.4 and Exercise 1.4: risk and return, $10,000 to invest.
fn print_risk_and_return(os: &mut impl Write) -> fmt::Result {
    let market = OneStepMarket::new(100.0, 110.0, 80.0, 100.0, 60.0, 0.8);
    writeln!(
        os,
        "\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, budget 10000 =="
    )?;
    print_risk_line(os, &market, "Mixed", &Portfolio { x: 50.0, y: 60.0 })?;
    print_risk_line(os, &market, "Bonds only", &Portfolio { x: 0.0, y: 100.0 })?;
    print_risk_line(os, &market, "Stock only", &Portfolio { x: 125.0, y: 0.0 })?;
    print_risk_line(
        os,
        &market,
        "Exercise 1.4 (50/50)",
        &market.split(10000.0, 0.5),
    )
}

/// Section 1.5: Example 1.5, Proposition 1.2, Exercises 1.5-1.6.
fn print_forwards(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 1.5 Forward contracts ==")?;
    writeln!(os, "Example 1.5: F={}", F4(80.0))?;
    for s1 in [84.0, 75.0] {
        writeln!(
            os,
            "  S(1)={}: long payoff {}, short payoff {}",
            F4(s1),
            F4(long_forward_payoff(s1, 80.0)),
            F4(short_forward_payoff(s1, 80.0))
        )?;
    }
    writeln!(
        os,
        "Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = {}",
        F4(forward_price(50.0, 100.0, 110.0))
    )?;

    let s0 = 34.0;
    let a0 = 100.0;
    let a1 = 112.0;
    let quoted = 38.60;
    writeln!(
        os,
        "Exercise 1.5: A=100->112, S(0)=34, quoted F={}, fair F={}",
        F4(quoted),
        F4(forward_price(s0, a0, a1))
    )?;
    let arb = forward_arbitrage(s0, a0, a1, quoted).expect("the forward is mispriced");
    writeln!(
        os,
        "  arbitrage (x,y,z)=({},{},{}) V(0)={} V(1)={}|{}|{} for S(1)=30|40|50",
        F4(arb.x),
        F4(arb.y),
        F4(arb.z),
        F4(value0(&arb, s0, a0)),
        F4(value1(&arb, 30.0, a1, quoted)),
        F4(value1(&arb, 40.0, a1, quoted)),
        F4(value1(&arb, 50.0, a1, quoted))
    )?;

    let foreign_rate = implied_foreign_rate(1.6, 1.50, 0.05);
    writeln!(
        os,
        "Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> sterling rate {}, bond \
         paying GBP 100 costs GBP {}",
        F4(foreign_rate),
        F4(100.0 / (1.0 + foreign_rate))
    )
}

/// Sections 1.6-1.7: options, Exercises 1.7-1.10.
fn print_options(os: &mut impl Write) -> fmt::Result {
    // p is left unspecified by the book.
    let market = OneStepMarket::new(100.0, 110.0, 100.0, 120.0, 80.0, 0.5);
    let s_up = market.s1(State::Up);
    let s_down = market.s1(State::Down);
    writeln!(
        os,
        "\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 =="
    )?;
    print_option_line(
        os,
        &market,
        "Call X=100",
        call_payoff(s_up, 100.0),
        call_payoff(s_down, 100.0),
    )?;
    for strike in [90.0, 110.0] {
        print_option_line(
            os,
            &market,
            &format!("Exercise 1.7 call X={strike:.0}"),
            call_payoff(s_up, strike),
            call_payoff(s_down, strike),
        )?;
    }
    for a1 in [105.0, 115.0] {
        let market_1_8 = OneStepMarket::new(100.0, a1, 100.0, 120.0, 80.0, 0.5);
        print_option_line(
            os,
            &market_1_8,
            &format!("Exercise 1.8 call X=100, A(1)={a1:.0}"),
            call_payoff(s_up, 100.0),
            call_payoff(s_down, 100.0),
        )?;
    }
    print_option_line(
        os,
        &market,
        "Exercise 1.9 put X=100",
        put_payoff(s_up, 100.0),
        put_payoff(s_down, 100.0),
    )?;

    let strike = 100.0;
    let call_up = call_payoff(s_up, strike);
    let call_down = call_payoff(s_down, strike);
    let put_up = put_payoff(s_up, strike);
    let put_down = put_payoff(s_down, strike);
    let call0 = market.price(call_up, call_down);
    let put0 = market.price(put_up, put_down);
    writeln!(
        os,
        "Risk-neutral p*={} -> C(0)={} P(0)={}",
        F4(market.risk_neutral_prob()),
        F4(market.risk_neutral_price(call_up, call_down)),
        F4(market.risk_neutral_price(put_up, put_down))
    )?;
    writeln!(
        os,
        "Put-call parity: C(0)-P(0)={} S(0)-X/(1+r)={}",
        F4(call0 - put0),
        F4(market.s0() - strike / (1.0 + market.r()))
    )?;

    writeln!(
        os,
        "\n== 1.7 Managing risk with options (section 1.6 market, budget 1000) =="
    )?;
    let budget = 1000.0;
    let num_shares = budget / market.s0();
    let num_calls = budget / call0;
    writeln!(
        os,
        "All in stock: {} shares -> V(1)={}|{} return {}|{}",
        F4(num_shares),
        F4(num_shares * s_up),
        F4(num_shares * s_down),
        F4(num_shares * s_up / budget - 1.0),
        F4(num_shares * s_down / budget - 1.0)
    )?;
    writeln!(
        os,
        "All in calls: {} calls  -> V(1)={}|{} return {}|{}",
        F4(num_calls),
        F4(num_calls * call_up),
        F4(num_calls * call_down),
        F4(num_calls * call_up / budget - 1.0),
        F4(num_calls * call_down / budget - 1.0)
    )?;
    let half_shares = budget / 2.0 / market.s0();
    let half_calls = budget / 2.0 / call0;
    let v_up = half_shares * s_up + half_calls * call_up;
    let v_down = half_shares * s_down + half_calls * call_down;
    writeln!(
        os,
        "Exercise 1.10 (50/50): {} shares + {} calls -> V(1)={}|{} return {}|{}",
        F4(half_shares),
        F4(half_calls),
        F4(v_up),
        F4(v_down),
        F4(v_up / budget - 1.0),
        F4(v_down / budget - 1.0)
    )
}

/// Section 1.7: hedging a future purchase with calls (strike 100),
/// Exercises 1.11-1.12.
fn print_hedging(os: &mut impl Write) -> fmt::Result {
    let market = OneStepMarket::new(100.0, 110.0, 100.0, 160.0, 40.0, 0.5);
    writeln!(
        os,
        "\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call X=100"
    )?;
    writeln!(
        os,
        "C(0)={} -> loan repaid at t=1: {}",
        F4(market.price(60.0, 0.0)),
        F4(market.price(60.0, 0.0) * (1.0 + market.r()))
    )?;
    for num_calls in [0.0, 1.0, 2.0] {
        writeln!(
            os,
            "Cost of one share with {num_calls:.0} calls: {}|{}",
            F4(hedged_purchase_cost(&market, 100.0, num_calls, State::Up)),
            F4(hedged_purchase_cost(&market, 100.0, num_calls, State::Down))
        )?;
    }
    for p in [0.25, 0.5, 0.75] {
        let market_p = OneStepMarket::new(100.0, 110.0, 100.0, 160.0, 40.0, p);
        let sigma_without = market_p.std_dev(
            hedged_purchase_cost(&market_p, 100.0, 0.0, State::Up),
            hedged_purchase_cost(&market_p, 100.0, 0.0, State::Down),
        );
        let sigma_with = market_p.std_dev(
            hedged_purchase_cost(&market_p, 100.0, 1.0, State::Up),
            hedged_purchase_cost(&market_p, 100.0, 1.0, State::Down),
        );
        writeln!(
            os,
            "Exercise 1.11/1.12 p={}: sigma without option={} with option={} ratio={}",
            F4(p),
            F4(sigma_without),
            F4(sigma_with),
            F4(sigma_with / sigma_without)
        )?;
    }
    Ok(())
}
