//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::binomial::crr_price;
use crate::options::{
    american_parity_bounds, bound_violation, butterfly_gap, call_payoff, european_call_bounds,
    european_put_bounds, parity_gap, profit, put_payoff,
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

/// The model price used throughout: r = 0.05, sigma = 0.2, 200 steps.
fn price(s0: f64, strike: f64, maturity: f64, is_call: bool, american: bool) -> f64 {
    crr_price(s0, strike, 0.05, 0.2, maturity, 200, is_call, american)
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 7: Options: General Properties (illustrative numbers, book formulas)"
    )?;
    print_definitions(os)?;
    print_parity(os)?;
    print_european_bounds(os)?;
    print_american_options(os)?;
    print_variables(os)?;
    print_time_value(os)
}

fn print_definitions(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 =="
    )?;
    for s_t in [80.0, 100.0, 120.0] {
        let call = call_payoff(s_t, 100.0);
        let put = put_payoff(s_t, 100.0);
        writeln!(
            os,
            "S(T)={}: call payoff={} profit long={} short={}; put payoff={} profit long={} \
             short={}",
            F4(s_t),
            F4(call),
            F4(profit(call, 10.0, 0.05, 1.0)),
            F4(-profit(call, 10.0, 0.05, 1.0)),
            F4(put),
            F4(profit(put, 5.0, 0.05, 1.0)),
            F4(-profit(put, 5.0, 0.05, 1.0))
        )?;
    }
    Ok(())
}

fn print_parity(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 r=0.05 T=1 =="
    )?;
    writeln!(
        os,
        "S(0) - X e^(-rT) = {}",
        F4(100.0 - 100.0 * (-0.05f64).exp())
    )?;
    for (call, put) in [(10.45, 5.57), (12.0, 5.0), (9.0, 6.0)] {
        let gap = parity_gap(call, put, 100.0, 100.0, 0.05, 1.0, 0.0);
        writeln!(
            os,
            "C={} P={}: gap={} arbitrage profit at T={}",
            F4(call),
            F4(put),
            F4(gap),
            F4(gap.abs() * 0.05f64.exp())
        )?;
    }
    let div = 2.0 * (-0.025f64).exp();
    writeln!(
        os,
        "Dividend 2 at t=0.5 (PV {}): C - P should be {}",
        F4(div),
        F4(-parity_gap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, div))
    )
}

fn print_european_bounds(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 =="
    )?;
    let call_bounds = european_call_bounds(100.0, 100.0, 0.05, 1.0, 0.0);
    let put_bounds = european_put_bounds(100.0, 100.0, 0.05, 1.0, 0.0);
    writeln!(
        os,
        "call in {}, put in {}",
        F4List(&[call_bounds.lo, call_bounds.hi]),
        F4List(&[put_bounds.lo, put_bounds.hi])
    )?;
    let model_call = price(100.0, 100.0, 1.0, true, false);
    writeln!(
        os,
        "Quoted call 3.50: violation {}; quoted put 96.00: violation {}; model call {}: \
         violation {}",
        F4(bound_violation(3.5, call_bounds)),
        F4(bound_violation(96.0, put_bounds)),
        F4(model_call),
        F4(bound_violation(model_call, call_bounds))
    )
}

fn print_american_options(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.3.2 American calls on a non-dividend paying stock (CRR, sigma=0.2, N=200) =="
    )?;
    for s0 in [90.0, 100.0, 120.0] {
        writeln!(
            os,
            "S(0)={}: C_E={} C_A={} exercise now={}",
            F4(s0),
            F4(price(s0, 100.0, 1.0, true, false)),
            F4(price(s0, 100.0, 1.0, true, true)),
            F4(call_payoff(s0, 100.0))
        )?;
    }
    writeln!(
        os,
        "\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) =="
    )?;
    for s0 in [90.0, 100.0, 120.0] {
        let put_e = price(s0, 100.0, 1.0, false, false);
        let put_a = price(s0, 100.0, 1.0, false, true);
        let call_a = price(s0, 100.0, 1.0, true, true);
        let b = american_parity_bounds(s0, 100.0, 0.05, 1.0);
        writeln!(
            os,
            "S(0)={}: P_E={} P_A={} C_A-P_A={} in {}",
            F4(s0),
            F4(put_e),
            F4(put_a),
            F4(call_a - put_a),
            F4List(&[b.lo, b.hi])
        )?;
    }
    Ok(())
}

fn print_variables(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.4.1 European options: dependence on X and S(0) =="
    )?;
    let strikes = [90.0, 100.0, 110.0];
    let calls: Vec<f64> = strikes
        .iter()
        .map(|&x| price(100.0, x, 1.0, true, false))
        .collect();
    let puts: Vec<f64> = strikes
        .iter()
        .map(|&x| price(100.0, x, 1.0, false, false))
        .collect();
    writeln!(
        os,
        "X={}: C={} P={}",
        F4List(&strikes),
        F4List(&calls),
        F4List(&puts)
    )?;
    writeln!(
        os,
        "C(90)-C(100)={} <= e^(-rT) x 10 = {}; butterfly gap={} (<= 0)",
        F4(calls[0] - calls[1]),
        F4(10.0 * (-0.05f64).exp()),
        F4(butterfly_gap(
            calls[0], calls[1], calls[2], 90.0, 100.0, 110.0
        ))
    )?;
    writeln!(
        os,
        "Quotes C=15, 10, 2: butterfly gap={} -> write 2 x C(100), buy C(90) and C(110)",
        F4(butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0))
    )?;
    let spots = [90.0, 100.0, 110.0];
    let calls_by_spot: Vec<f64> = spots
        .iter()
        .map(|&s| price(s, 100.0, 1.0, true, false))
        .collect();
    writeln!(os, "S(0)={}: C={}", F4List(&spots), F4List(&calls_by_spot))?;
    writeln!(os, "\n== 7.4.2 American options: dependence on T ==")?;
    let maturities = [0.25, 0.5, 1.0, 2.0];
    let american_puts: Vec<f64> = maturities
        .iter()
        .map(|&t| price(100.0, 100.0, t, false, true))
        .collect();
    writeln!(
        os,
        "T={}: P_A={}",
        F4List(&maturities),
        F4List(&american_puts)
    )
}

fn print_time_value(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 =="
    )?;
    for s0 in [80.0, 100.0, 120.0] {
        let call = price(s0, 100.0, 1.0, true, false);
        let put = price(s0, 100.0, 1.0, false, false);
        writeln!(
            os,
            "S(0)={}: call {} time value {}; put {} time value {}",
            F4(s0),
            F4(call),
            F4(call - call_payoff(s0, 100.0)),
            F4(put),
            F4(put - put_payoff(s0, 100.0))
        )?;
    }
    Ok(())
}
