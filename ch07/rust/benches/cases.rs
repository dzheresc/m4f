//! Benchmark cases for chapter 7, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch07::binomial::crr_price;
use m4f_ch07::options::{
    american_parity_bounds, bound_violation, butterfly_gap, call_payoff, european_call_bounds,
    european_put_bounds, parity_gap, profit, put_payoff,
};
use m4f_ch07::report::print_report;

fn strikes() -> Vec<f64> {
    let mut rng = Rng::new(20261031);
    table(|| rng.uniform(70.0, 130.0))
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Sections 7.1-7.4 (m4f_ch07::options).
    r.bench("CallPayoff", || {
        cycle(strikes(), |&x| call_payoff(100.0, x))
    });
    r.bench("Profit", || {
        cycle(strikes(), |&x| profit(put_payoff(100.0, x), 5.0, 0.05, 0.5))
    });
    r.bench("ParityGap", || {
        cycle(strikes(), |&x| {
            parity_gap(10.0, 5.0, 100.0, x, 0.05, 0.5, 1.0)
        })
    });
    r.bench("EuropeanBounds", || {
        cycle(strikes(), |&x| {
            bound_violation(8.0, european_call_bounds(100.0, x, 0.05, 0.5, 0.0))
                + bound_violation(4.0, european_put_bounds(100.0, x, 0.05, 0.5, 0.0))
        })
    });
    r.bench("AmericanParityBounds", || {
        cycle(strikes(), |&x| {
            let b = american_parity_bounds(100.0, x, 0.05, 0.5);
            b.lo + b.hi
        })
    });
    r.bench("ButterflyGap", || {
        cycle(strikes(), |&x| {
            butterfly_gap(12.0, 7.0, 3.0, x - 10.0, x, x + 10.0)
        })
    });

    // CRR trees (m4f_ch07::binomial).
    for (name, is_call, american) in [
        ("CrrEuropeanCall", true, false),
        ("CrrEuropeanPut", false, false),
        ("CrrAmericanCall", true, true),
        ("CrrAmericanPut", false, true),
    ] {
        for n in [10, 100, 500, 2000] {
            r.bench(&format!("{name}/{n}"), || {
                move || {
                    crr_price(
                        100.0,
                        black_box(100.0),
                        0.05,
                        0.2,
                        1.0,
                        n,
                        is_call,
                        american,
                    )
                }
            });
        }
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
