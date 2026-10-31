//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::binomial_pricing::{
    Tree, crr_call, early_exercise_nodes, european_price_sum, one_step_replication, option_tree,
    replicating_position, risk_neutral_prob, stock_price,
};
use crate::black_scholes::{bs_call, bs_call_delta, bs_put, crr_tree_price};
use crate::max;

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

fn call(strike: f64) -> impl Fn(f64) -> f64 {
    move |s| max(s - strike, 0.0)
}

fn put(strike: f64) -> impl Fn(f64) -> f64 {
    move |s| max(strike - s, 0.0)
}

fn reversed(row: &[f64]) -> Vec<f64> {
    row.iter().rev().copied().collect()
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 8: Option Pricing (illustrative numbers, book formulas)"
    )?;
    print_one_step(os)?;
    print_two_steps(os)?;
    print_general_model(os)?;
    print_american(os)?;
    print_black_scholes(os)
}

fn print_one_step(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 =="
    )?;
    let rep = one_step_replication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0);
    let q = risk_neutral_prob(0.2, -0.1, 0.1);
    writeln!(
        os,
        "Replicate with x={} shares, y={} in the money market: C(0)={}",
        F4(rep.x),
        F4(rep.y),
        F4(rep.price)
    )?;
    writeln!(
        os,
        "Risk-neutral: p*={}, C(0) = E*(C(1))/(1+r) = {}",
        F4(q),
        F4((q * 20.0 + (1.0 - q) * 0.0) / 1.1)
    )
}

fn print_two_steps(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 8.1.2 Two steps: same tree, call X=100 ==")?;
    let values: Tree = option_tree(100.0, 0.2, -0.1, 0.1, 2, &call(100.0), false);
    for n in 0..3 {
        let s: Vec<f64> = (0..n + 1)
            .rev()
            .map(|k| stock_price(100.0, 0.2, -0.1, n, k))
            .collect();
        writeln!(
            os,
            "n={n}: S={} C={}",
            F4List(&s),
            F4List(&reversed(&values[n as usize]))
        )?;
    }
    for (n, k) in [(0, 0), (1, 1), (1, 0)] {
        let (x, y) = replicating_position(100.0, 0.2, -0.1, 0.1, &values, n, k);
        writeln!(os, "node ({n},{k}): x={} y={}", F4(x), F4(y))?;
    }
    Ok(())
}

fn print_general_model(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 8.1.3 General N-step model: N=5, X=100 ==")?;
    let call_100 = call(100.0);
    let put_100 = put(100.0);
    let payoffs: [(&str, &dyn Fn(f64) -> f64); 2] = [("call", &call_100), ("put", &put_100)];
    for (name, payoff) in payoffs {
        writeln!(
            os,
            "{name}: backward induction={} sum formula={}",
            F4(option_tree(100.0, 0.2, -0.1, 0.1, 5, payoff, false)[0][0]),
            F4(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, payoff))
        )?;
    }
    // Bull spread 90/120.
    let spread = |s: f64| max(s - 90.0, 0.0).min(30.0);
    writeln!(
        os,
        "bull spread 90/120: {}",
        F4(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, &spread))
    )?;

    writeln!(os, "\n== 8.1.4 Cox-Ross-Rubinstein formula ==")?;
    for strike in [80.0, 100.0, 150.0] {
        let c = crr_call(100.0, strike, 0.2, -0.1, 0.1, 5);
        writeln!(
            os,
            "N=5 X={}: m={} C={} sum formula={}",
            F4(strike),
            c.m,
            F4(c.price),
            F4(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, &call(strike)))
        )?;
    }
    Ok(())
}

fn print_american(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put X=100 =="
    )?;
    let put_100 = put(100.0);
    let american = option_tree(100.0, 0.1, -0.1, 0.02, 4, &put_100, true);
    let european = option_tree(100.0, 0.1, -0.1, 0.02, 4, &put_100, false);
    writeln!(os, "P_A={} P_E={}", F4(american[0][0]), F4(european[0][0]))?;
    for (n, row) in american.iter().enumerate() {
        writeln!(os, "n={n}: P_A={}", F4List(&reversed(row)))?;
    }
    let mut nodes = String::new();
    for (n, k) in early_exercise_nodes(100.0, 0.1, -0.1, 0.02, 4, &put_100) {
        if !nodes.is_empty() {
            nodes.push_str(", ");
        }
        write!(
            nodes,
            "({n},{k}) S={}",
            F4(stock_price(100.0, 0.1, -0.1, n, k))
        )?;
    }
    writeln!(os, "Exercise early at (n,k): {nodes}")
}

fn print_black_scholes(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 =="
    )?;
    writeln!(
        os,
        "C={} P={} delta={}",
        F4(bs_call(100.0, 100.0, 0.05, 0.2, 1.0)),
        F4(bs_put(100.0, 100.0, 0.05, 0.2, 1.0)),
        F4(bs_call_delta(100.0, 100.0, 0.05, 0.2, 1.0))
    )?;
    for num_steps in [10, 50, 100, 500, 1000] {
        writeln!(
            os,
            "CRR N={num_steps}: C={} P={} P_A={}",
            F4(crr_tree_price(
                100.0, 100.0, 0.05, 0.2, 1.0, num_steps, true, false
            )),
            F4(crr_tree_price(
                100.0, 100.0, 0.05, 0.2, 1.0, num_steps, false, false
            )),
            F4(crr_tree_price(
                100.0, 100.0, 0.05, 0.2, 1.0, num_steps, false, true
            ))
        )?;
    }
    Ok(())
}
