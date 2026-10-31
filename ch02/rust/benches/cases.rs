//! Benchmark cases for chapter 2, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.
//!
//! Each interest function is a few arithmetic operations plus at most one
//! pow, exp or log, so each case cycles through a table of 1024 distinct
//! inputs to keep the compiler from folding the call into a constant.

use crate::harness::{Rng, Runner};
use m4f_ch02::interest::{
    annuity_future_value, annuity_present_value, continuous_from_periodic, continuous_value,
    effective_rate, loan_installment, periodic_from_continuous, periodic_from_periodic,
    periodic_value, simple_rate, simple_value,
};
use m4f_ch02::money_market::{
    coupon_bond_price, implied_rate, money_market_rollover, yield_to_maturity, zero_coupon_price,
};
use m4f_ch02::report::print_report;

/// Power of two: cheap wrap-around.
const NUM_INPUTS: usize = 1024;

#[derive(Clone, Copy)]
struct Inputs {
    /// Principal, value, payment or loan.
    amount: f64,
    /// Annual rate, 1% - 15%.
    r: f64,
    /// Years, 0.25 - 30.
    t: f64,
    /// Compounding periods per year.
    m: i32,
    /// Number of payments, 1 - 360.
    n: i32,
}

/// Converts a vector of exactly `NUM_INPUTS` items into a fixed-size array,
/// so that a masked index needs no bounds check (the C++ suite has none).
fn to_array<T>(v: Vec<T>) -> Box<[T; NUM_INPUTS]> {
    v.try_into().unwrap_or_else(|_| unreachable!())
}

fn table() -> Box<[Inputs; NUM_INPUTS]> {
    const PERIODS: [i32; 5] = [1, 2, 4, 12, 365];
    let mut rng = Rng::new(20261031);
    to_array(
        (0..NUM_INPUTS)
            .map(|_| Inputs {
                amount: rng.uniform(100.0, 100000.0),
                r: rng.uniform(0.01, 0.15),
                t: rng.uniform(0.25, 30.0),
                m: PERIODS[(rng.next_u64() % 5) as usize],
                n: rng.int(1, 360) as i32,
            })
            .collect(),
    )
}

/// Reproducible rates in 1% - 15%.
fn rates() -> Box<[f64; NUM_INPUTS]> {
    let mut rng = Rng::new(20261031);
    to_array((0..NUM_INPUTS).map(|_| rng.uniform(0.01, 0.15)).collect())
}

/// Returns an operation that applies `op` to the next row of `table()` on
/// each call.
fn over_inputs(mut op: impl FnMut(&Inputs) -> f64) -> impl FnMut() -> f64 {
    let table = table();
    let mut i = 0;
    move || {
        let r = op(&table[i]);
        i = (i + 1) & (NUM_INPUTS - 1);
        r
    }
}

/// Returns an operation that applies `op` to the next rate on each call.
fn over_rates(mut op: impl FnMut(f64) -> f64) -> impl FnMut() -> f64 {
    let rates = rates();
    let mut i = 0;
    move || {
        let r = op(rates[i]);
        i = (i + 1) & (NUM_INPUTS - 1);
        r
    }
}

/// Coupon counts for the bond benchmarks.
const BOND_LENGTHS: [i32; 5] = [3, 10, 30, 60, 360];

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Section 2.1, time value of money (m4f_ch02::interest).
    r.bench("SimpleValue", || {
        over_inputs(|i| simple_value(i.amount, i.r, i.t))
    });
    r.bench("SimpleRate", || {
        over_inputs(|i| simple_rate(i.amount, i.amount * (1.0 + i.r), i.t))
    });
    r.bench("PeriodicValue", || {
        over_inputs(|i| periodic_value(i.amount, i.r, i.m, i.t))
    });
    r.bench("EffectiveRate", || {
        over_inputs(|i| effective_rate(i.r, i.m))
    });
    r.bench("AnnuityPresentValue", || {
        over_inputs(|i| annuity_present_value(i.amount, i.r / 12.0, i.n))
    });
    r.bench("AnnuityFutureValue", || {
        over_inputs(|i| annuity_future_value(i.amount, i.r / 12.0, i.n))
    });
    r.bench("LoanInstallment", || {
        over_inputs(|i| loan_installment(i.amount, i.r / 12.0, i.n))
    });
    r.bench("ContinuousValue", || {
        over_inputs(|i| continuous_value(i.amount, i.r, i.t))
    });
    r.bench("ContinuousFromPeriodic", || {
        over_inputs(|i| continuous_from_periodic(i.r, i.m))
    });
    r.bench("PeriodicFromContinuous", || {
        over_inputs(|i| periodic_from_continuous(i.r, i.m))
    });
    r.bench("PeriodicFromPeriodic", || {
        over_inputs(|i| periodic_from_periodic(i.r, i.m, 12))
    });

    // Section 2.2, the money market (m4f_ch02::money_market).
    r.bench("ZeroCouponPricePeriodic", || {
        over_rates(|r| zero_coupon_price(r, 2.0, Some(2)))
    });
    r.bench("ZeroCouponPriceContinuous", || {
        over_rates(|r| zero_coupon_price(r, 2.0, None))
    });
    r.bench("ImpliedRatePeriodic", || {
        over_rates(|r| implied_rate(1.0 - r, 1.0, Some(2)))
    });
    // Per-period yield for coupon bonds of n periods, face 100, coupon 8.
    for n in BOND_LENGTHS {
        r.bench(&format!("CouponBondPrice/{n}"), || {
            over_rates(move |y| coupon_bond_price(100.0, 8.0, n, y))
        });
    }
    for n in BOND_LENGTHS {
        r.bench(&format!("YieldToMaturity/{n}"), || {
            // Prices of the same bonds at the generated yields, so every
            // solve has a different target.
            let prices = rates().map(|y| coupon_bond_price(100.0, 8.0, n, y));
            let mut i = 0;
            move || {
                let y = yield_to_maturity(prices[i], 100.0, 8.0, n);
                i = (i + 1) & (NUM_INPUTS - 1);
                y
            }
        });
    }
    for steps in [4, 120, 1024] {
        r.bench(&format!("MoneyMarketRollover/{steps}"), || {
            // 3-month bonds.
            let bond_prices: Vec<f64> = rates()[..steps].iter().map(|b| 1.0 - b / 4.0).collect();
            move || money_market_rollover(100.0, std::hint::black_box(&bond_prices))
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
