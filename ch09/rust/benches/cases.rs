//! Benchmark cases for chapter 9, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch09::black_scholes::{Greek, bs_greeks, bs_price, norm_cdf, norm_inv, norm_pdf};
use m4f_ch09::hedging::{
    Position, delta_greek_hedge, delta_hedge, portfolio_value, rebalance_path, total_greek,
};
use m4f_ch09::report::print_report;
use m4f_ch09::risk::{
    Lognormal, Strategy, all_in_stock, bull_spread, cdf, expected_call_payoff, expected_return,
    expected_value, initial_cost, mix, quantile, value_at_risk,
};

/// Random inputs for the Black-Scholes cases.
#[derive(Clone, Copy)]
struct Inputs {
    s: f64,
    sigma: f64,
    maturity: f64,
    prob: f64,
}

fn inputs() -> Vec<Inputs> {
    let mut rng = Rng::new(20261031);
    table(|| Inputs {
        s: rng.uniform(60.0, 140.0),
        sigma: rng.uniform(0.1, 0.5),
        maturity: rng.uniform(0.05, 3.0),
        prob: rng.uniform(0.001, 0.999),
    })
}

/// A book of `n` written options with mixed strikes, expiries and types.
fn book(n: usize) -> Vec<Position> {
    (0..n)
        .map(|i| Position {
            qty: -1.0 - (i % 3) as f64,
            strike: 80.0 + 40.0 * i as f64 / n as f64,
            expiry: 0.25 + 0.75 * (i % 4) as f64 / 3.0,
            is_call: i % 2 == 0,
        })
        .collect()
}

const MODEL: Lognormal = Lognormal {
    s0: 100.0,
    m: 0.12,
    sigma: 0.25,
    maturity: 0.5,
};

fn sample() -> Strategy {
    let stock = all_in_stock(10000.0, 100.0);
    let bull = bull_spread("bull", 10000.0, 100.0, 100.0, 120.0, 0.05, 0.25, 0.5);
    mix("mix", &stock, 0.5, &bull, 0.5)
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Black-Scholes (m4f_ch09::black_scholes).
    r.bench("NormCdf", || {
        cycle(inputs(), |x| norm_cdf(x.s / 20.0 - 5.0))
    });
    r.bench("NormPdf", || {
        cycle(inputs(), |x| norm_pdf(x.s / 20.0 - 5.0))
    });
    r.bench("NormInv", || cycle(inputs(), |x| norm_inv(x.prob)));
    for (name, is_call) in [("BsPriceCall", true), ("BsPricePut", false)] {
        r.bench(name, || {
            cycle(inputs(), move |x| {
                bs_price(x.s, 100.0, 0.05, x.sigma, x.maturity, is_call)
            })
        });
    }
    for (name, is_call) in [("BsGreeksCall", true), ("BsGreeksPut", false)] {
        r.bench(name, || {
            cycle(inputs(), move |x| {
                bs_greeks(x.s, 100.0, 0.05, x.sigma, x.maturity, is_call)
            })
        });
    }

    // Hedging (m4f_ch09::hedging).
    for n in [1, 10, 100] {
        r.bench(&format!("TotalGreek/{n}"), || {
            let b = book(n);
            move || total_greek(black_box(&b), 100.0, 0.05, 0.2, Greek::Gamma)
        });
    }
    for n in [1, 10, 100] {
        r.bench(&format!("DeltaHedge/{n}"), || {
            let b = book(n);
            move || delta_hedge(black_box(&b), 100.0, 0.05, 0.2)
        });
    }
    for n in [1, 10, 100] {
        r.bench(&format!("DeltaGammaHedge/{n}"), || {
            let b = book(n);
            let extra = Position {
                qty: 1.0,
                strike: 100.0,
                expiry: 0.5,
                is_call: true,
            };
            move || delta_greek_hedge(black_box(&b), &extra, 100.0, 0.05, 0.2, Greek::Gamma)
        });
    }
    for n in [1, 10, 100] {
        r.bench(&format!("PortfolioValue/{n}"), || {
            let hedge = delta_hedge(&book(n), 100.0, 0.05, 0.2);
            move || portfolio_value(black_box(&hedge), 101.0, 0.1, 0.05, 0.2)
        });
    }
    for steps in [52, 252] {
        r.bench(&format!("RebalancePath/{steps}"), || {
            let mut b = book(10);
            for p in &mut b {
                p.expiry = 1.5; // Outlives the path.
            }
            let mut rng = Rng::new(steps as u64);
            let sd = 0.2 / (steps as f64).sqrt();
            let mut prices = vec![100.0];
            for _ in 0..steps {
                let last = prices[prices.len() - 1];
                prices.push(last * rng.normal(0.0, sd).exp());
            }
            let dt = 1.0 / steps as f64;
            move || rebalance_path(black_box(&b), &prices, dt, 0.05, 0.2)
        });
    }

    // Value at Risk and speculation (m4f_ch09::risk).
    r.bench("Quantile", || move || quantile(&MODEL, black_box(0.05)));
    r.bench("Cdf", || move || cdf(&MODEL, black_box(90.0)));
    r.bench("ExpectedCallPayoff", || {
        move || expected_call_payoff(&MODEL, black_box(105.0))
    });
    r.bench("ExpectedValue", || {
        let st = sample();
        move || expected_value(black_box(&st), &MODEL, 0.05)
    });
    r.bench("InitialCost", || {
        let st = sample();
        move || initial_cost(black_box(&st), 100.0, 0.05, 0.25, 0.5)
    });
    r.bench("ValueAtRisk", || {
        let st = sample();
        move || value_at_risk(black_box(&st), &MODEL, 0.05, 0.25, 0.95)
    });
    r.bench("ExpectedReturn", || {
        let st = sample();
        move || expected_return(black_box(&st), &MODEL, 0.05, 0.25)
    });

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
