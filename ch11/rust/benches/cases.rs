//! Benchmark cases for chapter 11, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::Runner;
use m4f_ch11::derivatives::{bond_option, cap, caplet, floor, payer_swap, swap_rate};
use m4f_ch11::rate_tree::{
    Lattice, bond_lattice, bond_prices, constant_probs, implied_probs, money_market, rate_lattice,
};
use m4f_ch11::report::print_report;

/// Monthly steps.
const TAU: f64 = 1.0 / 12.0;

fn rates(n: i32) -> Lattice {
    rate_lattice(0.05, 0.002, -0.002, n)
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // The rate tree (m4f_ch11::rate_tree).
    for n in [10, 100, 1000] {
        r.bench(&format!("RateLattice/{n}"), || move || rates(black_box(n)));
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("BondLattice/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || bond_lattice(black_box(&rt), &p, TAU, n)
        });
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("ImpliedProbs/{n}"), || {
            let rt = rates(n);
            let bond = bond_lattice(&rt, &constant_probs(0.5, n), TAU, n);
            move || implied_probs(black_box(&bond), &rt, TAU)
        });
    }
    for n in [10, 50, 200] {
        r.bench(&format!("BondPrices/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || bond_prices(black_box(&rt), &p, TAU, n)
        });
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("MoneyMarket/{n}"), || {
            let rt = rates(n);
            let path: String = (0..n).map(|i| if i % 3 == 0 { 'u' } else { 'd' }).collect();
            move || money_market(black_box(&rt), TAU, &path)
        });
    }

    // Derivatives (m4f_ch11::derivatives).
    for (name, american) in [("BondOptionEuropean", false), ("BondOptionAmerican", true)] {
        for n in [10, 100, 1000] {
            r.bench(&format!("{name}/{n}"), || {
                let (rt, p) = (rates(n), constant_probs(0.5, n));
                move || bond_option(black_box(&rt), &p, TAU, n, n / 2, 0.97, false, american)
            });
        }
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("PayerSwap/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || payer_swap(black_box(&rt), &p, TAU, n, 0.004)
        });
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("Cap/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || cap(black_box(&rt), &p, TAU, n, 0.004)
        });
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("Floor/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || floor(black_box(&rt), &p, TAU, n, 0.004)
        });
    }
    for n in [10, 100, 1000] {
        r.bench(&format!("Caplet/{n}"), || {
            let (rt, p) = (rates(n), constant_probs(0.5, n));
            move || caplet(black_box(&rt), &p, TAU, n - 1, 0.004)
        });
    }
    for n in [10, 100] {
        r.bench(&format!("SwapRate/{n}"), || {
            let prices = bond_prices(&rates(n), &constant_probs(0.5, n), TAU, n);
            move || swap_rate(black_box(&prices))
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
