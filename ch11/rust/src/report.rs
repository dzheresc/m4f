//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::derivatives::{bond_option, cap, caplet, floating_rate, floor, payer_swap, swap_rate};
use crate::max;
use crate::rate_tree::{
    Lattice, bond_lattice, bond_prices, constant_probs, implied_prob, implied_probs, money_market,
    one_step_price, rate_lattice, yield_rate,
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

/// Lattice rows are stored k = 0..n (down to up); the report lists them
/// from the top node down.
fn reversed(row: &[f64]) -> Vec<f64> {
    row.iter().rev().copied().collect()
}

// The tree of Section 11.1: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau = 1.
const NUM_STEPS: i32 = 4;
const TAU: f64 = 1.0;

struct Tree {
    rates: Lattice,
    probs: Lattice,
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 11: Stochastic Interest Rates (illustrative numbers, book formulas)"
    )?;
    let tree = Tree {
        rates: rate_lattice(0.05, 0.01, -0.01, NUM_STEPS),
        probs: constant_probs(0.5, NUM_STEPS),
    };
    print_binomial_tree(os, &tree)?;
    print_bond_pricing(os, &tree)?;
    print_bond_options(os, &tree)?;
    print_swaps_caps_floors(os, &tree)
}

fn print_binomial_tree(os: &mut impl Write, tree: &Tree) -> fmt::Result {
    writeln!(
        os,
        "\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau=1, N=4 =="
    )?;
    for (n, row) in tree.rates.iter().enumerate() {
        let r = reversed(row);
        let one_step: Vec<f64> = r.iter().map(|&r| one_step_price(r, TAU)).collect();
        writeln!(os, "n={n}: r={} B(n,n+1)={}", F4List(&r), F4List(&one_step))?;
    }
    for path in ["uuuu", "udud", "dddd"] {
        writeln!(
            os,
            "Money market account along {path}: A={}",
            F4List(&money_market(&tree.rates, TAU, path))
        )?;
    }
    Ok(())
}

fn print_bond_pricing(os: &mut impl Write, tree: &Tree) -> fmt::Result {
    writeln!(os, "\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==")?;
    let b0 = bond_prices(&tree.rates, &tree.probs, TAU, NUM_STEPS);
    writeln!(os, "B(0,N) for N=1..4: {}", F4List(&b0))?;
    let yields: Vec<f64> = (0..NUM_STEPS)
        .map(|m| yield_rate(b0[m as usize], 0, m + 1, TAU))
        .collect();
    writeln!(os, "Term structure y(0,N): {}", F4List(&yields))?;
    let b4 = bond_lattice(&tree.rates, &tree.probs, TAU, NUM_STEPS);
    for (n, row) in b4.iter().enumerate() {
        writeln!(os, "B(n,4) at n={n}: {}", F4List(&reversed(row)))?;
    }
    let b3 = bond_lattice(&tree.rates, &tree.probs, TAU, 3);
    let b2 = bond_lattice(&tree.rates, &tree.probs, TAU, 2);
    let lattices: [(i32, &Lattice); 3] = [(2, &b2), (3, &b3), (4, &b4)];
    let mut y_up = Vec::new();
    let mut y_down = Vec::new();
    let mut returns = Vec::new();
    for (m, bond) in lattices {
        y_up.push(yield_rate(bond[1][1], 1, m, TAU));
        y_down.push(yield_rate(bond[1][0], 1, m, TAU));
        returns.push((0.5 * bond[1][1] + 0.5 * bond[1][0]) / bond[0][0]);
    }
    writeln!(
        os,
        "Term structure after one step: up node y(1,N)={} down node y(1,N)={}",
        F4List(&y_up),
        F4List(&y_down)
    )?;
    writeln!(
        os,
        "Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: {} = e^(r(0) tau) = {}",
        F4List(&returns),
        F4((0.05 * TAU).exp())
    )?;

    writeln!(
        os,
        "\n== 11.2.1 Risk-neutral probabilities implied by bond prices =="
    )?;
    for (m, bond) in [(4, &b4), (3, &b3), (2, &b2)] {
        let implied = implied_probs(bond, &tree.rates, TAU);
        let mut row = format!("from B(n,{m}):");
        for (n, level) in implied.iter().enumerate() {
            write!(row, " n={n}:{}", F4List(&reversed(level)))?;
        }
        writeln!(os, "{row}")?;
    }
    let probs6 = constant_probs(0.6, NUM_STEPS);
    let b4_p6 = bond_lattice(&tree.rates, &probs6, TAU, NUM_STEPS);
    writeln!(
        os,
        "Bonds priced with p*=0.6: B(0,4)={} (vs {}); implied p* at (0,0): {}",
        F4(b4_p6[0][0]),
        F4(b4[0][0]),
        F4(implied_prob(&b4_p6, &tree.rates, TAU, 0, 0))
    )?;
    let mut wrong = b2.clone();
    wrong[0][0] = b2[0][0] - 0.01;
    writeln!(
        os,
        "A two-year bond quoted at {} instead of {} implies p*={} at (0,0), inconsistent with the other bonds: arbitrage",
        F4(wrong[0][0]),
        F4(b2[0][0]),
        F4(implied_prob(&wrong, &tree.rates, TAU, 0, 0))
    )
}

fn print_bond_options(os: &mut impl Write, tree: &Tree) -> fmt::Result {
    writeln!(
        os,
        "\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 =="
    )?;
    let (rates, probs) = (&tree.rates, &tree.probs);
    let strike = 0.90;
    for is_call in [true, false] {
        let v = bond_option(rates, probs, TAU, NUM_STEPS, 2, strike, is_call, false);
        writeln!(
            os,
            "European {} X={}: payoff at n=2 {}, price {}",
            if is_call { "call" } else { "put" },
            F4(strike),
            F4List(&reversed(&v[2])),
            F4(v[0][0])
        )?;
    }
    let american = bond_option(rates, probs, TAU, NUM_STEPS, 2, strike, false, true);
    let b4 = bond_lattice(rates, probs, TAU, NUM_STEPS);
    let exercise: Vec<f64> = reversed(&b4[1])
        .iter()
        .map(|&b| max(strike - b, 0.0))
        .collect();
    writeln!(
        os,
        "American put X={}: price {} = immediate exercise X - B(0,4) = {}; values at n=1 {} = exercise values {}",
        F4(strike),
        F4(american[0][0]),
        F4(strike - b4[0][0]),
        F4List(&reversed(&american[1])),
        F4List(&exercise)
    )?;
    let american_call = bond_option(rates, probs, TAU, NUM_STEPS, 3, 0.85, true, true)[0][0];
    let european_call = bond_option(rates, probs, TAU, NUM_STEPS, 3, 0.85, true, false)[0][0];
    writeln!(
        os,
        "American call X=0.85 on the 4-year bond, expiry n=3: {} vs European {}",
        F4(american_call),
        F4(european_call)
    )?;
    writeln!(
        os,
        "Call on the 2-year bond expiring at n=1, X=0.95: {}",
        F4(bond_option(rates, probs, TAU, 2, 1, 0.95, true, false)[0][0])
    )
}

fn print_swaps_caps_floors(os: &mut impl Write, tree: &Tree) -> fmt::Result {
    let (rates, probs) = (&tree.rates, &tree.probs);
    writeln!(
        os,
        "\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, 4 annual payments =="
    )?;
    for n in 0..NUM_STEPS {
        let floating: Vec<f64> = (0..n + 1)
            .rev()
            .map(|k| floating_rate(rates, TAU, n, k))
            .collect();
        writeln!(os, "n={n}: L={}", F4List(&floating))?;
    }
    let k_swap = swap_rate(&bond_prices(rates, probs, TAU, NUM_STEPS));
    writeln!(
        os,
        "Swap rate K = (1 - B(0,4)) / sum B(0,n) = {}",
        F4(k_swap)
    )?;
    let swap = payer_swap(rates, probs, TAU, NUM_STEPS, k_swap);
    writeln!(
        os,
        "Payer swap value at K: {}; at n=1: {}",
        F4(swap[0][0]),
        F4List(&reversed(&swap[1]))
    )?;
    for fixed_rate in [0.04, 0.05, 0.06] {
        writeln!(
            os,
            "Payer swap at K={}: {}",
            F4(fixed_rate),
            F4(payer_swap(rates, probs, TAU, NUM_STEPS, fixed_rate)[0][0])
        )?;
    }

    writeln!(
        os,
        "\n== 11.3.3 Caps and floors on the same floating rate =="
    )?;
    for strike in [0.04, k_swap, 0.06] {
        let c = cap(rates, probs, TAU, NUM_STEPS, strike)[0][0];
        let f = floor(rates, probs, TAU, NUM_STEPS, strike)[0][0];
        let swap_value = payer_swap(rates, probs, TAU, NUM_STEPS, strike)[0][0];
        writeln!(
            os,
            "K={}: cap={} floor={} cap-floor={} = payer swap {}",
            F4(strike),
            F4(c),
            F4(f),
            F4(c - f),
            F4(swap_value)
        )?;
    }
    let mut caplets = Vec::new();
    let mut sum = 0.0;
    for n in 0..NUM_STEPS {
        let c = caplet(rates, probs, TAU, n, 0.05);
        caplets.push(c);
        sum += c;
    }
    writeln!(
        os,
        "Caplets at K=0.05 by fixing date n=0..3: {}, sum {} = cap {}",
        F4List(&caplets),
        F4(sum),
        F4(cap(rates, probs, TAU, NUM_STEPS, 0.05)[0][0])
    )
}
