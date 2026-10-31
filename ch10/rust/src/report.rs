//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::bonds::{
    Bond, Flow, coupon_bond, duration, dynamic_hedge, immunise, immunising_weights,
    investment_value, log_return, price, zero_coupon,
};
use crate::term_structure::{
    coupon_bond_price, forward_rate, forward_rates, implied_price, money_market_account,
    prices_from_forward_rates, yield_rate, yield_to_maturity,
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

const Y0: f64 = 0.06;
const V0: f64 = 100000.0;

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 10: Variable Interest Rates (illustrative numbers, book formulas)"
    )?;
    print_single_bonds(os)?;
    print_duration(os)?;
    let a = coupon_bond(100.0, 4.0, 5, 1.0);
    let b = coupon_bond(100.0, 8.0, 10, 1.0);
    print_portfolios(os, &a, &b)?;
    print_dynamic_hedging(os, &a, &b)?;
    print_term_structure(
        os,
        &[0.5, 1.0, 1.5, 2.0, 2.5, 3.0],
        &[0.978, 0.953, 0.926, 0.898, 0.870, 0.842],
    )
}

fn print_single_bonds(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 years, y=0.06 continuous =="
    )?;
    let bond = coupon_bond(100.0, 5.0, 4, 1.0);
    writeln!(os, "Price at y=0.06: {}", F4(price(&bond, Y0, 0.0)))?;
    writeln!(
        os,
        "Yield jumps right after purchase; value of the investment at horizon t (coupons reinvested):"
    )?;
    for y in [0.05, 0.06, 0.07] {
        let mut row = format!("y={}:", F4(y));
        for t in 1..5 {
            write!(
                row,
                " t={t}:{}",
                F4(investment_value(&bond, y, f64::from(t)))
            )?;
        }
        writeln!(os, "{row}")?;
    }
    writeln!(os, "Annualised log return over [0,t]:")?;
    for y in [0.05, 0.06, 0.07] {
        let mut row = format!("y={}:", F4(y));
        for t in 1..5 {
            write!(row, " t={t}:{}", F4(log_return(&bond, Y0, y, f64::from(t))))?;
        }
        writeln!(os, "{row}")?;
    }
    Ok(())
}

fn print_duration(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P =="
    )?;
    let bond = coupon_bond(100.0, 5.0, 4, 1.0);
    let d = duration(&bond, Y0, 0.0);
    writeln!(
        os,
        "Coupon bond: D={} years; zero-coupon 4y: D={}",
        F4(d),
        F4(duration(&zero_coupon(100.0, 4.0), Y0, 0.0))
    )?;
    let h = 1e-4;
    writeln!(
        os,
        "dP/dy by finite difference: {}, -D P = {}",
        F4((price(&bond, Y0 + h, 0.0) - price(&bond, Y0 - h, 0.0)) / (2.0 * h)),
        F4(-d * price(&bond, Y0, 0.0))
    )?;
    writeln!(
        os,
        "Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first order): {}",
        F4List(&[
            investment_value(&bond, 0.05, d),
            investment_value(&bond, 0.06, d),
            investment_value(&bond, 0.07, d)
        ])
    )?;
    let named: [(&str, Bond); 3] = [
        ("5y 4%", coupon_bond(100.0, 4.0, 5, 1.0)),
        ("10y 8%", coupon_bond(100.0, 8.0, 10, 1.0)),
        ("10y zero", zero_coupon(100.0, 10.0)),
    ];
    for (name, b) in &named {
        writeln!(
            os,
            "{name}: price={} D={} at y=0.06; D at y=0.08: {}",
            F4(price(b, Y0, 0.0)),
            F4(duration(b, Y0, 0.0)),
            F4(duration(b, 0.08, 0.0))
        )?;
    }
    Ok(())
}

fn print_portfolios(os: &mut impl Write, a: &[Flow], b: &[Flow]) -> fmt::Result {
    writeln!(
        os,
        "\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 years, y=0.06 =="
    )?;
    let d_a = duration(a, Y0, 0.0);
    let d_b = duration(b, Y0, 0.0);
    let (w_a, w_b) = immunising_weights(d_a, d_b, 5.0);
    writeln!(
        os,
        "D_A={} D_B={}: weights w_A={} w_B={} give D={}",
        F4(d_a),
        F4(d_b),
        F4(w_a),
        F4(w_b),
        F4(w_a * d_a + w_b * d_b)
    )?;
    let p = immunise(V0, a, b, Y0, 0.0, 5.0);
    writeln!(
        os,
        "Invest {}: {} of A and {} of B",
        F4(V0),
        F4(p.q1),
        F4(p.q2)
    )?;
    let target = V0 * (Y0 * 5.0).exp();
    writeln!(os, "Target at t=5 if y stays 0.06: {}", F4(target))?;
    writeln!(
        os,
        "Value at t=5 after an immediate jump of y (coupons reinvested at the new yield):"
    )?;
    for y in [0.04, 0.05, 0.06, 0.07, 0.08] {
        let v = p.q1 * investment_value(a, y, 5.0) + p.q2 * investment_value(b, y, 5.0);
        writeln!(
            os,
            "y={}: portfolio {} (shortfall {}), B alone {}, A alone {}",
            F4(y),
            F4(v),
            F4(target - v),
            F4(V0 / price(b, Y0, 0.0) * investment_value(b, y, 5.0)),
            F4(V0 / price(a, Y0, 0.0) * investment_value(a, y, 5.0))
        )?;
    }
    Ok(())
}

fn print_dynamic_hedging(os: &mut impl Write, a: &[Flow], b: &[Flow]) -> fmt::Result {
    writeln!(
        os,
        "\n== 10.1.4 Dynamic hedging: re-immunise every half year along a yield path =="
    )?;
    let path = [
        (0.0, 0.06),
        (0.5, 0.065),
        (1.0, 0.07),
        (1.5, 0.06),
        (2.0, 0.05),
        (2.5, 0.055),
        (3.0, 0.065),
        (3.5, 0.07),
        (4.0, 0.075),
        (4.5, 0.07),
        (5.0, 0.065),
    ];
    let rebalanced = dynamic_hedge(V0, a, b, 5.0, &path, true);
    for st in &rebalanced {
        writeln!(
            os,
            "t={} y={}: value={} q_A={} q_B={} D={}",
            F4(st.t),
            F4(st.y),
            F4(st.value),
            F4(st.q1),
            F4(st.q2),
            F4(st.duration)
        )?;
    }
    let fixed = dynamic_hedge(V0, a, b, 5.0, &path, false);
    writeln!(
        os,
        "At t=5: rebalanced {}, not rebalanced {}, target {}",
        F4(rebalanced[rebalanced.len() - 1].value),
        F4(fixed[fixed.len() - 1].value),
        F4(V0 * (Y0 * 5.0).exp())
    )
}

fn print_term_structure(os: &mut impl Write, maturities: &[f64], prices: &[f64]) -> fmt::Result {
    writeln!(os, "\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==")?;
    let forwards = forward_rates(maturities, prices);
    for i in 0..maturities.len() {
        writeln!(
            os,
            "T={}: B={} y(0,T)={} f(0,T-0.5,T)={}",
            F4(maturities[i]),
            F4(prices[i]),
            F4(yield_rate(prices[i], maturities[i])),
            F4(forwards[i])
        )?;
    }
    writeln!(
        os,
        "Prices rebuilt from the forward rates: {}",
        F4List(&prices_from_forward_rates(maturities, &forwards))
    )?;
    let mut avg = 0.0;
    for f in &forwards[2..6] {
        avg += f;
    }
    writeln!(
        os,
        "f(0,1,3)={} = average of the half-year forwards {}",
        F4(forward_rate(prices[1], 1.0, prices[5], 3.0)),
        F4(avg / 4.0)
    )?;
    let mut row = String::from("Implied B(1,T) = B(0,T)/B(0,1):");
    for i in 2..maturities.len() {
        write!(
            row,
            " T={}:{}",
            F4(maturities[i]),
            F4(implied_price(prices[1], prices[i]))
        )?;
    }
    writeln!(os, "{row}")?;
    writeln!(
        os,
        "FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)={}: buy 1000 one-year bonds (cost {}), sell {} two-year bonds; repay {}",
        F4(forward_rate(prices[1], 1.0, prices[3], 2.0)),
        F4(1000.0 * prices[1]),
        F4(1000.0 * prices[1] / prices[3]),
        F4(1000.0 * forward_rate(prices[1], 1.0, prices[3], 2.0).exp())
    )?;
    let flow_times = [1.0, 2.0, 3.0];
    let flow_amounts = [6.0, 6.0, 106.0];
    let p = coupon_bond_price(&flow_times, &flow_amounts, maturities, prices);
    writeln!(
        os,
        "3-year 6% coupon bond under the term structure: price {}, yield to maturity {} (between y(0,1)={} and y(0,3)={})",
        F4(p),
        F4(yield_to_maturity(p, &flow_times, &flow_amounts)),
        F4(yield_rate(prices[1], 1.0)),
        F4(yield_rate(prices[5], 3.0))
    )?;

    writeln!(
        os,
        "\n== 10.2.2 Money market account: rolling over half-year bonds at the forward rates =="
    )?;
    let account = money_market_account(maturities, prices);
    for i in 0..maturities.len() {
        writeln!(
            os,
            "A({})={} = 1/B(0,T)={}",
            F4(maturities[i]),
            F4(account[i]),
            F4(1.0 / prices[i])
        )?;
    }
    let mut sum_f = 0.0;
    for f in &forwards {
        sum_f += f * 0.5;
    }
    writeln!(
        os,
        "Short rates realised = forward rates: {}; A(3)=exp(sum f dt)={}",
        F4List(&forwards),
        F4(sum_f.exp())
    )?;
    writeln!(
        os,
        "1 unit at t=0 in the 3-year bond: {} at t=3, same as the money market account {}",
        F4(1.0 / prices[5]),
        F4(account[5])
    )
}
