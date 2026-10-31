//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::forwards::{
    forward_arbitrage_profit, forward_price, forward_price_dividends, forward_price_yield,
    long_forward_value,
};
use crate::futures::{futures_tree, hedged_std, margin_account, min_variance_hedge_ratio};

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

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 6: Forward and Futures Contracts (illustrative numbers, book formulas)"
    )?;
    let f0 = forward_price(100.0, 0.08, 0.75);
    print_forward_price(os, f0)?;
    print_forward_value(os, f0)?;
    print_futures_pricing(os)?;
    print_hedging(os)
}

fn print_forward_price(os: &mut impl Write, f0: f64) -> fmt::Result {
    writeln!(
        os,
        "\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 T=0.75 =="
    )?;
    writeln!(os, "No dividends: F(0,T)={}", F4(f0))?;
    writeln!(
        os,
        "Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)={}",
        F4(forward_price_dividends(
            100.0,
            0.08,
            0.75,
            &[(2.0, 0.25), (2.0, 0.5)]
        ))
    )?;
    writeln!(
        os,
        "Dividend yield 3%: F(0,T)={}",
        F4(forward_price_yield(100.0, 0.08, 0.03, 0.75))
    )?;
    writeln!(
        os,
        "Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: F={}",
        F4(forward_price_yield(1.25, 0.05, 0.03, 1.0))
    )?;
    for quoted in [108.0, 104.0] {
        let side = if quoted > f0 {
            "cash and carry: buy stock, sell forward"
        } else {
            "reverse: short stock, buy forward"
        };
        writeln!(
            os,
            "Quoted F={}: {side}, profit at T={}",
            F4(quoted),
            F4(forward_arbitrage_profit(quoted, 100.0, 0.08, 0.75))
        )?;
    }
    Ok(())
}

fn print_forward_value(os: &mut impl Write, f0: f64) -> fmt::Result {
    writeln!(
        os,
        "\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) e^(-r(T-t)) =="
    )?;
    for st in [104.0, 98.0] {
        let ft = forward_price(st, 0.08, 0.25);
        writeln!(
            os,
            "t=0.5 S(t)={}: F(t,T)={} long V(t)={} short V(t)={}",
            F4(st),
            F4(ft),
            F4(long_forward_value(f0, ft, 0.08, 0.25)),
            F4(-long_forward_value(f0, ft, 0.08, 0.25))
        )?;
    }
    Ok(())
}

fn print_futures_pricing(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 d=-0.05 r=0.03 N=3 =="
    )?;
    let tree = futures_tree(100.0, 0.10, -0.05, 0.03, 3);
    for n in 0..4 {
        let mut futures = Vec::new();
        let mut forward = Vec::new();
        for k in (0..n + 1).rev() {
            futures.push(tree[n as usize][k as usize]);
            forward.push(
                100.0
                    * 1.1f64.powf(f64::from(k))
                    * 0.95f64.powf(f64::from(n - k))
                    * 1.03f64.powf(f64::from(3 - n)),
            );
        }
        writeln!(
            os,
            "n={n}: futures={} forward={}",
            F4List(&futures),
            F4List(&forward)
        )?;
    }
    writeln!(
        os,
        "Marking to market: 100 units, initial margin 1000, maintenance 750"
    )?;
    let prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0];
    let days = margin_account(&prices, 100.0, 1000.0, 750.0);
    for (i, day) in days.iter().enumerate() {
        let n = i + 1;
        writeln!(
            os,
            "day {n}: f={} balance={} margin call={}",
            F4(prices[n]),
            F4(day.balance),
            F4(day.call)
        )?;
    }
    Ok(())
}

fn print_hedging(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 6.2.2 Hedging with futures: four equally likely scenarios =="
    )?;
    let ds = [-6.0, -1.0, 2.0, 5.0];
    let df = [-5.0, -2.0, 3.0, 4.0];
    let p = [0.25, 0.25, 0.25, 0.25];
    let h = min_variance_hedge_ratio(&ds, &df, &p);
    writeln!(os, "dS={} df={}", F4List(&ds), F4List(&df))?;
    writeln!(
        os,
        "h*={}; std of spot change: unhedged {}, h=1 {}, h* {}",
        F4(h),
        F4(hedged_std(0.0, &ds, &df, &p)),
        F4(hedged_std(1.0, &ds, &df, &p)),
        F4(hedged_std(h, &ds, &df, &p))
    )?;
    writeln!(
        os,
        "Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 or 95 -> cost {}",
        F4List(&[112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)])
    )
}
