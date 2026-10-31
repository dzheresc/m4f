//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::no_arbitrage::{
    arbitrage_strategy, expected_discounted_value, is_arbitrage_free, one_step_values,
    risk_neutral_probs,
};
use crate::strategies::{discounted_gains, gains, self_financing, self_financing_multi};

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

/// Hold one share plus one more for every down move so far.
fn contrarian(_n: i32, moves: &[i32]) -> f64 {
    let ups: i32 = moves.iter().sum();
    1.0 + f64::from(moves.len() as i32 - ups)
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 4: Discrete Time Market Models (illustrative numbers, book formulas)"
    )?;
    print_strategies(os)?;
    print_no_arbitrage(os)?;
    print_binomial_tree(os)?;
    print_fundamental_theorem(os)?;
    print_extended_models(os)
}

fn print_strategies(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 4.1.1 Investment strategies: self-financing x(n+1)S(n) + y(n+1)A(n) = V(n) =="
    )?;
    let s = [100.0, 110.0, 104.5, 114.95];
    let a: Vec<f64> = (0..4).map(|n| 100.0 * 1.03f64.powf(f64::from(n))).collect();
    let x = [5.0, 8.0, 3.0];
    let path = self_financing(1000.0, &s, &a, &x);
    let (y, values) = (&path.y, &path.values);
    writeln!(os, "V(0)=1000, A(n)=100*1.03^n")?;
    for n in 1..4 {
        writeln!(
            os,
            "n={n}: S={} A={} x={} y={} V={}",
            F4(s[n]),
            F4(a[n]),
            F4(x[n - 1]),
            F4(y[n - 1]),
            F4(values[n])
        )?;
    }
    writeln!(
        os,
        "V(3)-V(0)={} gains sum={}",
        F4(values[3] - values[0]),
        F4(gains(&s, &a, &x, y))
    )?;
    writeln!(
        os,
        "V~(3)-V~(0)={} discounted gains={}",
        F4(values[3] / a[3] - values[0] / a[0]),
        F4(discounted_gains(&s, &a, &x))
    )
}

fn print_no_arbitrage(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, K in {{0.10, -0.05}} =="
    )?;
    for r in [0.12, 0.03, -0.05] {
        let returns = [0.10, -0.05];
        if is_arbitrage_free(&returns, r) {
            writeln!(os, "r={}: no arbitrage", F4(r))?;
        } else {
            let (x, y) =
                arbitrage_strategy(&returns, r, 100.0, 100.0).expect("the market admits arbitrage");
            writeln!(
                os,
                "r={}: arbitrage x={} y={} V(0)={} V(1)={}",
                F4(r),
                F4(x),
                F4(y),
                F4(x * 100.0 + y * 100.0),
                F4List(&one_step_values(x, y, &returns, r, 100.0, 100.0))
            )?;
        }
    }
    Ok(())
}

fn print_binomial_tree(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, x(n) = 1 + #downs so far =="
    )?;
    let q = (0.03 - -0.05) / (0.10 - -0.05);
    writeln!(
        os,
        "V(0)=100: E*(V~(4))={} (p*={}), E(V~(4))={} (p=0.6)",
        F4(expected_discounted_value(
            100.0,
            0.10,
            -0.05,
            0.03,
            4,
            100.0,
            &contrarian,
            q
        )),
        F4(q),
        F4(expected_discounted_value(
            100.0,
            0.10,
            -0.05,
            0.03,
            4,
            100.0,
            &contrarian,
            0.6
        ))
    )
}

fn print_fundamental_theorem(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, K in {{-0.10, 0, \
         0.10}} =="
    )?;
    let returns = [-0.10, 0.0, 0.10];
    for r in [-0.15, 0.02, 0.05, 0.10] {
        match risk_neutral_probs(&returns, r) {
            None => writeln!(os, "r={}: arbitrage, no risk-neutral probability", F4(r))?,
            Some(q) => {
                let mut sum = 0.0;
                for (qi, k) in q.iter().zip(&returns) {
                    sum += qi * k;
                }
                writeln!(os, "r={}: q={} sum qK={}", F4(r), F4List(&q), F4(sum))?;
            }
        }
    }
    Ok(())
}

fn print_extended_models(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, A(n)=1.05^n =="
    )?;
    let s = [vec![50.0, 55.0, 52.0, 58.0], vec![20.0, 19.0, 21.0, 22.0]];
    let a: Vec<f64> = (0..4).map(|n| 1.05f64.powf(f64::from(n))).collect();
    let x = [vec![10.0, 12.0, 8.0], vec![20.0, 15.0, 25.0]];
    let dividends = [vec![0.0, 0.0, 0.0, 0.0], vec![0.0, 0.0, 2.0, 0.0]];
    let with_dividend = self_financing_multi(1000.0, &s, &a, &x, Some(&dividends));
    for n in 1..4 {
        writeln!(
            os,
            "n={n}: x1={} x2={} y={} V={}",
            F4(x[0][n - 1]),
            F4(x[1][n - 1]),
            F4(with_dividend.y[n - 1]),
            F4(with_dividend.values[n])
        )?;
    }
    let without = self_financing_multi(1000.0, &s, &a, &x, None);
    writeln!(
        os,
        "Without the dividend V(3)={}; it adds {} = 2 x 15 x 1.05",
        F4(without.values[3]),
        F4(with_dividend.values[3] - without.values[3])
    )
}
