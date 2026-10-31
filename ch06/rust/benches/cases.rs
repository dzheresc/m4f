//! Benchmark cases for chapter 6, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch06::forwards::{
    forward_arbitrage_profit, forward_price, forward_price_dividends, forward_price_yield,
    long_forward_value,
};
use m4f_ch06::futures::{futures_tree, hedged_std, margin_account, min_variance_hedge_ratio};
use m4f_ch06::report::print_report;

fn rates() -> Vec<f64> {
    let mut rng = Rng::new(20261031);
    table(|| rng.uniform(0.0, 0.10))
}

fn path(days: usize) -> Vec<f64> {
    let mut rng = Rng::new(days as u64);
    let mut path = vec![100.0];
    for i in 0..days {
        path.push(path[i] * rng.normal(0.0, 0.015).exp());
    }
    path
}

struct Scenarios {
    ds: Vec<f64>,
    df: Vec<f64>,
    p: Vec<f64>,
}

fn make_scenarios(n: usize) -> Scenarios {
    let mut rng = Rng::new(n as u64);
    let mut s = Scenarios {
        ds: Vec::new(),
        df: Vec::new(),
        p: vec![1.0 / n as f64; n],
    };
    for _ in 0..n {
        let df = rng.normal(0.0, 2.0);
        s.df.push(df);
        s.ds.push(0.9 * df + 0.3 * rng.normal(0.0, 2.0));
    }
    s
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Section 6.1 (m4f_ch06::forwards).
    r.bench("ForwardPrice", || {
        cycle(rates(), |&r| forward_price(100.0, r, 0.75))
    });
    r.bench("ForwardPriceYield", || {
        cycle(rates(), |&r| forward_price_yield(100.0, r, 0.02, 0.75))
    });
    r.bench("ForwardArbitrageProfit", || {
        cycle(rates(), |&r| {
            forward_arbitrage_profit(105.0, 100.0, r, 0.75)
        })
    });
    r.bench("LongForwardValue", || {
        cycle(rates(), |&r| long_forward_value(100.0, 104.0, r, 0.5))
    });
    for n in [4, 100] {
        r.bench(&format!("ForwardPriceDividends/{n}"), || {
            let dividends: Vec<(f64, f64)> =
                (1..n + 1).map(|i| (0.5, f64::from(i) / 4.0)).collect();
            let maturity = f64::from(n) / 4.0;
            cycle(rates(), move |&r| {
                forward_price_dividends(100.0, r, maturity, &dividends)
            })
        });
    }

    // Section 6.2 (m4f_ch06::futures).
    for n in [3, 10, 100, 1000] {
        r.bench(&format!("FuturesTree/{n}"), || {
            move || futures_tree(100.0, 0.02, -0.01, 0.005, black_box(n))
        });
    }
    for days in [252, 2520] {
        r.bench(&format!("MarginAccount/{days}"), || {
            let prices = path(days);
            move || margin_account(black_box(&prices), 100.0, 1000.0, 700.0)
        });
    }
    for n in [3, 100, 10000] {
        r.bench(&format!("MinVarianceHedgeRatio/{n}"), || {
            let s = make_scenarios(n);
            move || min_variance_hedge_ratio(black_box(&s.ds), &s.df, &s.p)
        });
    }
    for n in [3, 100, 10000] {
        r.bench(&format!("HedgedStd/{n}"), || {
            let s = make_scenarios(n);
            move || hedged_std(0.9, black_box(&s.ds), &s.df, &s.p)
        });
    }

    // End to end: the full chapter report, written to memory.
    r.bench("PrintReport", || {
        let mut out = String::new();
        move || {
            out.clear();
            print_report(&mut out).expect("writing to a String cannot fail");
            out.len()
        }
    });
}
