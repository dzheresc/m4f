//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::black_scholes::{Greek, bs_greeks, bs_price, norm_inv};
use crate::hedging::{
    Hedge, Position, delta_greek_hedge, delta_hedge, portfolio_value, rebalance_path, total_greek,
};
use crate::risk::{
    Lognormal, Strategy, all_in_calls, all_in_stock, breakeven, bull_spread, cdf, expected_return,
    expected_stock, expected_value, initial_cost, mix, prob_loss, put_hedged_holding, quantile,
    terminal_value, unhedged_holding, value_at_risk,
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

/// "[a, b, c]" with each entry formatted by `F4`.
struct F4List<'a>(&'a [f64]);

impl Display for F4List<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("[")?;
        for (i, &v) in self.0.iter().enumerate() {
            if i > 0 {
                f.write_str(", ")?;
            }
            write!(f, "{}", F4(v))?;
        }
        f.write_str("]")
    }
}

fn leg_qtys(st: &Strategy) -> Vec<f64> {
    st.legs.iter().map(|l| l.qty).collect()
}

fn greek_name(g: Greek) -> &'static str {
    match g {
        Greek::Delta => "delta",
        Greek::Gamma => "gamma",
        Greek::Theta => "theta",
        Greek::Vega => "vega",
        Greek::Rho => "rho",
    }
}

// The option position of Section 9.1: S(0)=100 X=100 r=0.05 sigma=0.2 T=0.5.
const S0: f64 = 100.0;
const STRIKE: f64 = 100.0;
const RATE: f64 = 0.05;
const SIGMA: f64 = 0.2;
const MATURITY: f64 = 0.5;

const MOVES: [f64; 7] = [90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0];

const WRITTEN: [Position; 1] = [Position {
    qty: -1000.0,
    strike: STRIKE,
    expiry: MATURITY,
    is_call: true,
}];

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 9: Financial Engineering (illustrative numbers, book formulas)"
    )?;
    print_delta_hedging(os)?;
    print_greeks(os)?;
    print_greek_hedging(os)?;
    print_value_at_risk(
        os,
        &Lognormal {
            s0: 100.0,
            m: 0.08,
            sigma: 0.3,
            maturity: 0.5,
        },
    )?;
    print_speculating(os)?;
    print_bull_case(os)
}

fn print_delta_hedging(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 r=0.05 sigma=0.2 =="
    )?;
    let call = bs_price(S0, STRIKE, RATE, SIGMA, MATURITY, true);
    let h = delta_hedge(&WRITTEN, S0, RATE, SIGMA);
    writeln!(
        os,
        "Call price C={}, delta={}",
        F4(call),
        F4(bs_greeks(S0, STRIKE, RATE, SIGMA, MATURITY, true).delta)
    )?;
    writeln!(
        os,
        "Premium received {}; buy {} shares, money market {} (negative = borrow)",
        F4(1000.0 * call),
        F4(h.shares),
        F4(h.money)
    )?;
    writeln!(
        os,
        "Value of the position right after a jump in S (t=0): naked writer vs delta hedged"
    )?;
    for s in MOVES {
        let naked = 1000.0 * call - 1000.0 * bs_price(s, STRIKE, RATE, SIGMA, MATURITY, true);
        writeln!(
            os,
            "S={}: naked {} hedged {}",
            F4(s),
            F4(naked),
            F4(portfolio_value(&h, s, 0.0, RATE, SIGMA))
        )?;
    }
    writeln!(os, "Same jumps one week later (t=1/52), no rebalancing:")?;
    let mut row = String::from(" ");
    for s in MOVES {
        write!(
            row,
            " S={}:{}",
            F4(s),
            F4(portfolio_value(&h, s, 1.0 / 52.0, RATE, SIGMA))
        )?;
    }
    writeln!(os, "{row}")?;

    writeln!(
        os,
        "\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each week =="
    )?;
    let path = [100.0, 103.0, 101.0, 98.0, 102.0, 104.0];
    for st in rebalance_path(&WRITTEN, &path, 1.0 / 52.0, RATE, SIGMA) {
        writeln!(
            os,
            "t={} S={}: shares={} money={} value={}",
            F4(st.t),
            F4(st.s),
            F4(st.shares),
            F4(st.money),
            F4(st.value)
        )?;
    }
    Ok(())
}

fn print_greeks(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 =="
    )?;
    for is_call in [true, false] {
        let g = bs_greeks(S0, STRIKE, RATE, SIGMA, MATURITY, is_call);
        writeln!(
            os,
            "{}: price={} delta={} gamma={} theta={} vega={} rho={}",
            if is_call { "call" } else { "put" },
            F4(g.price),
            F4(g.delta),
            F4(g.gamma),
            F4(g.theta),
            F4(g.vega),
            F4(g.rho)
        )?;
    }
    let mut row = String::from("Position Greeks of the written calls:");
    for g in [
        Greek::Delta,
        Greek::Gamma,
        Greek::Theta,
        Greek::Vega,
        Greek::Rho,
    ] {
        write!(
            row,
            " {}={}",
            greek_name(g),
            F4(total_greek(&WRITTEN, S0, RATE, SIGMA, g))
        )?;
    }
    writeln!(os, "{row}")
}

fn print_greek_hedging(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5 =="
    )?;
    let extra = Position {
        qty: 0.0,
        strike: 110.0,
        expiry: MATURITY,
        is_call: true,
    };
    let h = delta_hedge(&WRITTEN, S0, RATE, SIGMA);
    let h_gamma = delta_greek_hedge(&WRITTEN, &extra, S0, RATE, SIGMA, Greek::Gamma);
    let h_vega = delta_greek_hedge(&WRITTEN, &extra, S0, RATE, SIGMA, Greek::Vega);
    let hedges: [(&str, &Hedge); 3] = [
        ("delta", &h),
        ("delta-gamma", &h_gamma),
        ("delta-vega", &h_vega),
    ];
    for (label, hedge) in hedges {
        let second = hedge.options.get(1).map_or(0.0, |p| p.qty);
        writeln!(
            os,
            "{label}: shares={} calls X=110: {} money={} gamma={} vega={}",
            F4(hedge.shares),
            F4(second),
            F4(hedge.money),
            F4(total_greek(&hedge.options, S0, RATE, SIGMA, Greek::Gamma)),
            F4(total_greek(&hedge.options, S0, RATE, SIGMA, Greek::Vega))
        )?;
    }
    writeln!(os, "Value after an immediate jump in S:")?;
    for s in MOVES {
        writeln!(
            os,
            "S={}: delta {} delta-gamma {} delta-vega {}",
            F4(s),
            F4(portfolio_value(&h, s, 0.0, RATE, SIGMA)),
            F4(portfolio_value(&h_gamma, s, 0.0, RATE, SIGMA)),
            F4(portfolio_value(&h_vega, s, 0.0, RATE, SIGMA))
        )?;
    }
    writeln!(
        os,
        "Value after an immediate change in sigma (S unchanged):"
    )?;
    for sigma in [0.15, 0.2, 0.25, 0.3] {
        writeln!(
            os,
            "sigma={}: delta {} delta-gamma {} delta-vega {}",
            F4(sigma),
            F4(portfolio_value(&h, S0, 0.0, RATE, sigma)),
            F4(portfolio_value(&h_gamma, S0, 0.0, RATE, sigma)),
            F4(portfolio_value(&h_vega, S0, 0.0, RATE, sigma))
        )?;
    }

    writeln!(
        os,
        "\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 =="
    )?;
    for (label, hedge) in hedges {
        writeln!(
            os,
            "{label}: {}",
            F4(portfolio_value(hedge, 97.0, 1.0 / 52.0, RATE, 0.22))
        )?;
    }
    writeln!(
        os,
        "Theta: value change over one week with S and sigma unchanged"
    )?;
    for (label, hedge) in hedges {
        writeln!(
            os,
            "{label}: {}  (theta x dt = {})",
            F4(portfolio_value(hedge, S0, 1.0 / 52.0, RATE, SIGMA)),
            F4(total_greek(&hedge.options, S0, RATE, SIGMA, Greek::Theta) / 52.0)
        )?;
    }
    Ok(())
}

fn print_value_at_risk(os: &mut impl Write, model: &Lognormal) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 (lognormal) =="
    )?;
    let stock = unhedged_holding(1.0);
    writeln!(os, "E S(T)={}", F4(expected_stock(model)))?;
    for conf in [0.9, 0.95, 0.99] {
        writeln!(
            os,
            "confidence {}: z={} S quantile={} VaR={}",
            F4(conf),
            F4(norm_inv(1.0 - conf)),
            F4(quantile(model, 1.0 - conf)),
            F4(value_at_risk(&stock, model, RATE, SIGMA, conf))
        )?;
    }
    writeln!(
        os,
        "Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:"
    )?;
    for strike in [85.0, 90.0, 95.0, 100.0] {
        let st = put_hedged_holding(&format!("put X={}", strike as i32), 1.0, strike, 1.0);
        writeln!(
            os,
            "X={}: put price={} initial value={} VaR={} E V(T)={} expected return={}",
            F4(strike),
            F4(bs_price(100.0, strike, RATE, 0.3, 0.5, false)),
            F4(initial_cost(&st, 100.0, RATE, 0.3, 0.5)),
            F4(value_at_risk(&st, model, RATE, 0.3, 0.95)),
            F4(expected_value(&st, model, RATE)),
            F4(expected_return(&st, model, RATE, 0.3))
        )?;
    }
    let half = put_hedged_holding("half hedged X=95", 1.0, 95.0, 0.5);
    writeln!(
        os,
        "Half hedged X=95: VaR={} expected return={}",
        F4(value_at_risk(&half, model, RATE, 0.3, 0.95)),
        F4(expected_return(&half, model, RATE, 0.3))
    )?;

    writeln!(
        os,
        "\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) target 100000 =="
    )?;
    let big = unhedged_holding(10000.0);
    writeln!(
        os,
        "Unhedged: VaR={} expected return={}",
        F4(value_at_risk(&big, model, RATE, 0.3, 0.95)),
        F4(expected_return(&big, model, RATE, 0.3))
    )?;
    let mut chosen = String::new();
    for strike in [80.0, 85.0, 90.0, 95.0, 100.0, 105.0] {
        let st = put_hedged_holding(&format!("put X={}", strike as i32), 10000.0, strike, 1.0);
        let var = value_at_risk(&st, model, RATE, 0.3, 0.95);
        let cost = 10000.0 * bs_price(100.0, strike, RATE, 0.3, 0.5, false);
        writeln!(
            os,
            "puts X={}: cost={} VaR={} expected return={}",
            F4(strike),
            F4(cost),
            F4(var),
            F4(expected_return(&st, model, RATE, 0.3))
        )?;
        if chosen.is_empty() && var <= 100000.0 {
            chosen = st.name;
        }
    }
    writeln!(os, "Cheapest hedge meeting the target: {chosen}")
}

fn print_speculating(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 T=0.5, r=0.05 =="
    )?;
    let spec = Lognormal {
        s0: 100.0,
        m: 0.10,
        sigma: 0.25,
        maturity: 0.5,
    };
    let stock = all_in_stock(10000.0, 100.0);
    let calls100 = all_in_calls("calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.5);
    let calls110 = all_in_calls("calls X=110", 10000.0, 100.0, 110.0, RATE, 0.25, 0.5);
    let half_mix = mix("half stock, half calls X=100", &stock, 0.5, &calls100, 0.5);
    let strategies = [stock, calls100, calls110, half_mix];
    writeln!(
        os,
        "C(X=100)={} C(X=110)={}",
        F4(bs_price(100.0, 100.0, RATE, 0.25, 0.5, true)),
        F4(bs_price(100.0, 110.0, RATE, 0.25, 0.5, true))
    )?;
    for st in &strategies {
        writeln!(
            os,
            "{}: shares={} options={}",
            st.name,
            F4(st.shares),
            F4List(&leg_qtys(st))
        )?;
    }
    let scenarios = [80.0, 90.0, 100.0, 110.0, 120.0, 130.0];
    writeln!(
        os,
        "Terminal value by scenario S(T)={}:",
        F4List(&scenarios)
    )?;
    for st in &strategies {
        let values: Vec<f64> = scenarios
            .iter()
            .map(|&s| terminal_value(st, s, RATE, 0.5))
            .collect();
        writeln!(os, "{}: {}", st.name, F4List(&values))?;
    }
    writeln!(
        os,
        "Expected return, probability of loss, breakeven S(T), VaR at 95%:"
    )?;
    for st in &strategies {
        writeln!(
            os,
            "{}: E ret={} P(loss)={} breakeven={} VaR={}",
            st.name,
            F4(expected_return(st, &spec, RATE, 0.25)),
            F4(prob_loss(st, &spec, RATE, 0.25)),
            F4(breakeven(st, &spec, RATE, 0.25)),
            F4(value_at_risk(st, &spec, RATE, 0.25, 0.95))
        )?;
    }
    Ok(())
}

fn print_bull_case(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months (m=0.4 sigma=0.25 T=0.25) =="
    )?;
    let bull = Lognormal {
        s0: 100.0,
        m: 0.4,
        sigma: 0.25,
        maturity: 0.25,
    };
    let strategies = [
        all_in_stock(10000.0, 100.0),
        all_in_calls("calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.25),
        bull_spread(
            "bull spread 100/115",
            10000.0,
            100.0,
            100.0,
            115.0,
            RATE,
            0.25,
            0.25,
        ),
    ];
    writeln!(
        os,
        "E S(T)={} P(S(T)>115)={}",
        F4(expected_stock(&bull)),
        F4(1.0 - cdf(&bull, 115.0))
    )?;
    for st in &strategies {
        writeln!(
            os,
            "{}: options={} E ret={} P(loss)={} VaR={} value at S(T)=115: {}",
            st.name,
            F4List(&leg_qtys(st)),
            F4(expected_return(st, &bull, RATE, 0.25)),
            F4(prob_loss(st, &bull, RATE, 0.25)),
            F4(value_at_risk(st, &bull, RATE, 0.25, 0.95)),
            F4(terminal_value(st, 115.0, RATE, 0.25))
        )?;
    }
    let mut row = String::from("If instead the stock stays flat (S(T)=100):");
    for st in &strategies {
        write!(
            row,
            " {}={}",
            st.name,
            F4(terminal_value(st, 100.0, RATE, 0.25))
        )?;
    }
    writeln!(os, "{row}")
}
