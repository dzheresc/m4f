//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::binomial_tree::{BinomialTree, binomial_prob};
use crate::other_models::{
    TrinomialModel, binomial_expected_price, binomial_prob_above, lognormal_expected_price,
    lognormal_prob_above,
};
use crate::returns::{
    compound_return, expectation, log_return, log_returns, one_step_return, returns, variance,
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
    writeln!(
        os,
        "Chapter 3: Risky Assets (illustrative numbers, book formulas)"
    )?;
    print_returns(os)?;
    print_expected_return(os)?;
    let tree = BinomialTree {
        s0: 100.0,
        u: 0.10,
        d: -0.05,
        r: 0.03,
    };
    print_binomial_tree(os, &tree)?;
    print_risk_neutral(os, &tree)?;
    print_martingale(os, &tree)?;
    print_trinomial(os)?;
    print_continuous_limit(os)
}

fn print_returns(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), k(n) = ln(S(n) / S(n-1)) =="
    )?;
    let path = [45.0, 48.0, 47.5, 50.0];
    let returns = returns(&path);
    let log_returns = log_returns(&path);
    for n in 1..path.len() {
        writeln!(
            os,
            "n={n}: S={} K={} k={}",
            F4(path[n]),
            F4(returns[n - 1]),
            F4(log_returns[n - 1])
        )?;
    }
    writeln!(
        os,
        "K(0,3) from S: {}, from (1+K(1))(1+K(2))(1+K(3))-1: {}",
        F4(one_step_return(path[0], path[3], 0.0)),
        F4(compound_return(&returns))
    )?;
    writeln!(
        os,
        "k(0,3) from S: {}, k(1)+k(2)+k(3): {}",
        F4(log_return(path[0], path[3])),
        F4(log_returns[0] + log_returns[1] + log_returns[2])
    )?;
    writeln!(
        os,
        "S(0)=45, S(1)=48 with dividend 2: K(1)={}",
        F4(one_step_return(45.0, 48.0, 2.0))
    )
}

fn print_expected_return(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 3.1.2 Expected return ==")?;
    let prices = [52.0, 48.0, 40.0];
    let probs = [0.25, 0.5, 0.25];
    let returns: Vec<f64> = prices
        .iter()
        .map(|&s1| one_step_return(45.0, s1, 0.0))
        .collect();
    let expected_return = expectation(&returns, &probs);
    writeln!(
        os,
        "S(0)=45, S(1) in {{52, 48, 40}} with p={{0.25, 0.5, 0.25}}: E(K)={} sd(K)={}",
        F4(expected_return),
        F4(variance(&returns, &probs).sqrt())
    )?;
    writeln!(
        os,
        "E(S(1)) = (1 + E(K)) S(0) = {}, directly {}",
        F4((1.0 + expected_return) * 45.0),
        F4(expectation(&prices, &probs))
    )?;
    writeln!(
        os,
        "Independent steps, E(K(1))=0.10, E(K(2))=0.05: E(K(0,2)) = {}",
        F4(compound_return(&[0.10, 0.05]))
    )
}

fn print_binomial_tree(os: &mut impl Write, tree: &BinomialTree) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 =="
    )?;
    for n in 0..4 {
        write!(os, "n={n}: ")?;
        for k in (0..=n).rev() {
            if k != n {
                write!(os, " ")?;
            }
            write!(
                os,
                "{}({})",
                F4(tree.price(n, k)),
                F4(binomial_prob(n, k, 0.6))
            )?;
        }
        writeln!(os)?;
    }
    writeln!(
        os,
        "E(K(n))={} E(S(3))={} (1+E(K))^3 S(0)={}",
        F4(tree.expected_return(0.6)),
        F4(tree.expected_price(3, 0.6)),
        F4((1.0 + tree.expected_return(0.6)).powf(3.0) * 100.0)
    )
}

fn print_risk_neutral(os: &mut impl Write, tree: &BinomialTree) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) =="
    )?;
    let q = tree.risk_neutral_prob();
    writeln!(
        os,
        "d < r < u: {}, p*={}, E*(K)={}",
        if tree.is_arbitrage_free() {
            "yes"
        } else {
            "no"
        },
        F4(q),
        F4(tree.expected_return(q))
    )?;
    writeln!(
        os,
        "E*(S(1))={} (1+r)S(0)={}; E*(S(3))={} (1+r)^3 S(0)={}",
        F4(tree.expected_price(1, q)),
        F4(1.03 * 100.0),
        F4(tree.expected_price(3, q)),
        F4(1.03f64.powf(3.0) * 100.0)
    )
}

fn print_martingale(os: &mut impl Write, tree: &BinomialTree) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) =="
    )?;
    let mut worst: f64 = 0.0;
    for n in 0..3 {
        for k in 0..=n {
            worst =
                worst.max((tree.conditional_discounted(n, k) - tree.discounted_price(n, k)).abs());
        }
    }
    for k in [1, 0] {
        writeln!(
            os,
            "node (1,{k}): S~(1)={} E*(S~(2)|.)={}",
            F4(tree.discounted_price(1, k)),
            F4(tree.conditional_discounted(1, k))
        )?;
    }
    writeln!(os, "Largest deviation over all nodes n<3: {}", F4(worst))
}

fn print_trinomial(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 =="
    )?;
    let model = TrinomialModel {
        u: 0.10,
        m: 0.0,
        d: -0.10,
        r: 0.02,
    };
    writeln!(
        os,
        "Risk-neutral probabilities exist for 0 < p_m < {}",
        F4(model.max_middle_prob())
    )?;
    for p_mid in [0.2, 0.4, 0.6] {
        let probs = model.risk_neutral_probs(p_mid);
        let call = probs.p_up * 10.0 / 1.02;
        writeln!(
            os,
            "p_m={}: p_u={} p_d={} E*(K)={} call(K=100) price={}",
            F4(probs.p_mid),
            F4(probs.p_up),
            F4(probs.p_down),
            F4(probs.p_up * 0.10 + p_mid * 0.0 + probs.p_down * -0.10),
            F4(call)
        )?;
    }
    Ok(())
}

fn print_continuous_limit(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 =="
    )?;
    for num_steps in [1, 10, 100, 1000, 10000] {
        writeln!(
            os,
            "N={num_steps}: E(S(T))={} P(S(T)>110)={}",
            F4(binomial_expected_price(100.0, 0.05, 0.2, 1.0, num_steps)),
            F4(binomial_prob_above(100.0, 0.05, 0.2, 1.0, num_steps, 110.0))
        )?;
    }
    writeln!(
        os,
        "Limit: E(S(T))={} P(S(T)>110)={}",
        F4(lognormal_expected_price(100.0, 0.05, 0.2, 1.0)),
        F4(lognormal_prob_above(100.0, 0.05, 0.2, 1.0, 110.0))
    )
}
