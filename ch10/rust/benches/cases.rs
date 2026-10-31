//! Benchmark cases for chapter 10, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner};
use m4f_ch10::bonds::{Bond, coupon_bond, duration, dynamic_hedge, immunise, log_return, price};
use m4f_ch10::report::print_report;
use m4f_ch10::term_structure::{
    coupon_bond_price, forward_rates, money_market_account, prices_from_forward_rates,
    yield_to_maturity,
};

/// Coupon bonds with 3 annual flows (the book's size), 10 annual, 60
/// semi-annual (30 years) and 360 monthly (30 years).
fn make_bond(n: i32) -> Bond {
    let dt = if n <= 10 {
        1.0
    } else if n <= 60 {
        0.5
    } else {
        1.0 / 12.0
    };
    coupon_bond(100.0, 8.0 * dt, n, dt)
}

/// A smooth upward-sloping zero curve at n annual maturities.
struct Curve {
    maturities: Vec<f64>,
    prices: Vec<f64>,
}

fn make_curve(n: usize) -> Curve {
    let maturities: Vec<f64> = (1..n + 1).map(|i| i as f64).collect();
    let prices = maturities
        .iter()
        .map(|&t| (-(0.03 + 0.02 * (1.0 - (-t / 5.0).exp())) * t).exp())
        .collect();
    Curve { maturities, prices }
}

/// A 5 per cent coupon at every maturity, plus the face at the last.
fn amounts(c: &Curve) -> Vec<f64> {
    let mut a = vec![5.0; c.maturities.len()];
    let last = a.len() - 1;
    a[last] += 100.0;
    a
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Bonds under a flat yield (m4f_ch10::bonds).
    for n in [3, 10, 60, 360] {
        r.bench(&format!("Price/{n}"), || {
            let bond = make_bond(n);
            move || price(&bond, black_box(0.06), 0.0)
        });
    }
    for n in [3, 10, 60, 360] {
        r.bench(&format!("Duration/{n}"), || {
            let bond = make_bond(n);
            move || duration(&bond, black_box(0.06), 0.0)
        });
    }
    for n in [3, 10, 60, 360] {
        r.bench(&format!("LogReturn/{n}"), || {
            let bond = make_bond(n);
            move || log_return(&bond, 0.06, black_box(0.07), 1.5)
        });
    }
    r.bench("Immunise", || {
        let b1 = coupon_bond(100.0, 8.0, 2, 1.0);
        let b2 = coupon_bond(100.0, 6.0, 10, 1.0);
        move || immunise(1000.0, &b1, &b2, black_box(0.06), 0.0, 5.0)
    });
    // Rebalancing an immunised portfolio at 5, 52 (weekly) and 260 (daily)
    // dates over a 5-year horizon along a random yield path.
    for steps in [5, 52, 260] {
        r.bench(&format!("DynamicHedge/{steps}"), || {
            let b1 = coupon_bond(100.0, 8.0, 2, 1.0);
            let b2 = coupon_bond(100.0, 6.0, 10, 1.0);
            let mut rng = Rng::new(steps as u64);
            let sd = 0.01 / (steps as f64 / 5.0).sqrt();
            let mut path = vec![(0.0, 0.06)];
            for i in 1..steps + 1 {
                let y = path[path.len() - 1].1 + rng.normal(0.0, sd);
                path.push((5.0 * i as f64 / steps as f64, y));
            }
            move || dynamic_hedge(1000.0, &b1, &b2, 5.0, black_box(&path), true)
        });
    }

    // Term structure (m4f_ch10::term_structure).
    for n in [3, 30, 360] {
        r.bench(&format!("ForwardRates/{n}"), || {
            let c = make_curve(n);
            move || forward_rates(black_box(&c.maturities), &c.prices)
        });
    }
    for n in [3, 30, 360] {
        r.bench(&format!("PricesFromForwardRates/{n}"), || {
            let c = make_curve(n);
            let f = forward_rates(&c.maturities, &c.prices);
            move || prices_from_forward_rates(black_box(&c.maturities), &f)
        });
    }
    for n in [3, 30, 360] {
        r.bench(&format!("CouponBondPrice/{n}"), || {
            let c = make_curve(n);
            let a = amounts(&c);
            move || coupon_bond_price(black_box(&c.maturities), &a, &c.maturities, &c.prices)
        });
    }
    for n in [3, 30, 360] {
        r.bench(&format!("YieldToMaturity/{n}"), || {
            let c = make_curve(n);
            let a = amounts(&c);
            let p = coupon_bond_price(&c.maturities, &a, &c.maturities, &c.prices);
            move || yield_to_maturity(black_box(p), &c.maturities, &a)
        });
    }
    for n in [3, 30, 360] {
        r.bench(&format!("MoneyMarketAccount/{n}"), || {
            let c = make_curve(n);
            move || money_market_account(&c.maturities, black_box(&c.prices))
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
