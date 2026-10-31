//! Benchmark cases for chapter 8, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.
//!
//! `BinomialSum`, `EuropeanPriceSum` and `CrrCall` stop at N = 1000 like
//! the C++ suite.

use std::hint::black_box;

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch08::binomial_pricing::{
    binomial_sum, crr_call, early_exercise_nodes, european_price_sum, option_tree,
    replicating_position,
};
use m4f_ch08::black_scholes::{bs_call, bs_call_delta, bs_put, crr_tree_price, norm_cdf};
use m4f_ch08::report::print_report;

/// (u, d, r) per step of a one-year CRR tree with r = 5% and sigma = 20%.
fn crr(num_steps: i32) -> (f64, f64, f64) {
    let tau = 1.0 / f64::from(num_steps);
    (
        (0.2 * tau.sqrt()).exp() - 1.0,
        (-0.2 * tau.sqrt()).exp() - 1.0,
        (0.05 * tau).exp() - 1.0,
    )
}

/// `std::max(s - 100.0, 0.0)`, as in the C++ suite.
fn call(s: f64) -> f64 {
    let v = s - 100.0;
    if v < 0.0 { 0.0 } else { v }
}

/// `std::max(100.0 - s, 0.0)`.
fn put(s: f64) -> f64 {
    let v = 100.0 - s;
    if v < 0.0 { 0.0 } else { v }
}

/// Random (s, sigma, maturity) triples.
fn inputs() -> Vec<(f64, f64, f64)> {
    let mut rng = Rng::new(20261031);
    table(|| {
        (
            rng.uniform(60.0, 140.0),
            rng.uniform(0.1, 0.5),
            rng.uniform(0.05, 3.0),
        )
    })
}

const TREE_STEPS: [i32; 4] = [10, 100, 500, 2000];
const SUM_STEPS: [i32; 3] = [10, 100, 1000];

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Binomial pricing (m4f_ch08::binomial_pricing).
    for n in TREE_STEPS {
        r.bench(&format!("OptionTreeEuropean/{n}"), || {
            let (u, d, rr) = crr(n);
            move || option_tree(100.0, u, d, rr, black_box(n), &put, false)
        });
    }
    for n in TREE_STEPS {
        r.bench(&format!("OptionTreeAmerican/{n}"), || {
            let (u, d, rr) = crr(n);
            move || option_tree(100.0, u, d, rr, black_box(n), &put, true)
        });
    }
    for n in TREE_STEPS {
        r.bench(&format!("EarlyExerciseNodes/{n}"), || {
            let (u, d, rr) = crr(n);
            move || early_exercise_nodes(100.0, u, d, rr, black_box(n), &put)
        });
    }
    r.bench("ReplicatingPosition", || {
        let (u, d, rr) = crr(100);
        let tree = option_tree(100.0, u, d, rr, 100, &call, false);
        let mut k = 0;
        move || {
            let position = replicating_position(100.0, u, d, rr, black_box(&tree), 50, k);
            k = (k + 1) % 51;
            position
        }
    });
    for n in SUM_STEPS {
        r.bench(&format!("BinomialSum/{n}"), || {
            move || binomial_sum(0.5, black_box(n), n / 2)
        });
    }
    for n in SUM_STEPS {
        r.bench(&format!("EuropeanPriceSum/{n}"), || {
            let (u, d, rr) = crr(n);
            move || european_price_sum(100.0, u, d, rr, black_box(n), &call)
        });
    }
    for n in SUM_STEPS {
        r.bench(&format!("CrrCall/{n}"), || {
            let (u, d, rr) = crr(n);
            move || crr_call(100.0, 100.0, u, d, rr, black_box(n))
        });
    }

    // Black-Scholes (m4f_ch08::black_scholes).
    r.bench("NormCdf", || {
        cycle(inputs(), |&(s, _, _)| norm_cdf(s / 20.0 - 5.0))
    });
    r.bench("BsCall", || {
        cycle(inputs(), |&(s, sigma, t)| bs_call(s, 100.0, 0.05, sigma, t))
    });
    r.bench("BsPut", || {
        cycle(inputs(), |&(s, sigma, t)| bs_put(s, 100.0, 0.05, sigma, t))
    });
    r.bench("BsCallDelta", || {
        cycle(inputs(), |&(s, sigma, t)| {
            bs_call_delta(s, 100.0, 0.05, sigma, t)
        })
    });
    for n in TREE_STEPS {
        r.bench(&format!("CrrTreePriceEuropean/{n}"), || {
            move || crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, black_box(n), true, false)
        });
    }
    for n in TREE_STEPS {
        r.bench(&format!("CrrTreePriceAmerican/{n}"), || {
            move || crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, black_box(n), false, true)
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
