//! Benchmark cases for chapter 1, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.
//!
//! The one-step functions are a handful of floating-point operations, so
//! those cases cycle through a table of 1024 distinct markets. That keeps the
//! compiler from folding the work into a constant, as in the C++ suite.
//! Results that borrow from the market are passed through `black_box`
//! before the closure returns, which is what `DoNotOptimize` does in C++.

use std::hint::black_box;

use crate::harness::{Rng, Runner};
use m4f_ch01::forward::forward_arbitrage;
use m4f_ch01::fx_arbitrage::{FxMarket, FxQuote};
use m4f_ch01::one_step::{OneStepMarket, State, call_payoff, hedged_purchase_cost};
use m4f_ch01::report::print_report;

/// Power of two: cheap wrap-around.
const NUM_MARKETS: usize = 1024;

#[derive(Clone, Copy)]
struct MarketInputs {
    a0: f64,
    a1: f64,
    s0: f64,
    s_up: f64,
    s_down: f64,
    p: f64,
}

/// Random but reproducible markets around the book's examples
/// (A(0)=100, S(0)=100); about half admit arbitrage.
fn inputs() -> Box<[MarketInputs; NUM_MARKETS]> {
    let mut rng = Rng::new(20261031);
    let v: Vec<MarketInputs> = (0..NUM_MARKETS)
        .map(|_| {
            let rate = rng.uniform(0.0, 0.15);
            let up = rng.uniform(0.0, 0.5);
            let d = rng.uniform(-0.4, 0.1);
            let p = rng.uniform(0.05, 0.95);
            let u = d + 0.01 + up;
            MarketInputs {
                a0: 100.0,
                a1: 100.0 * (1.0 + rate),
                s0: 100.0,
                s_up: 100.0 * (1.0 + u),
                s_down: 100.0 * (1.0 + d),
                p,
            }
        })
        .collect();
    v.try_into().unwrap_or_else(|_| unreachable!())
}

/// The markets of `inputs()`. A fixed-size array lets the compiler see that
/// a masked index is in bounds, so the loop has no bounds check (the C++
/// suite has none either).
fn markets() -> Box<[OneStepMarket; NUM_MARKETS]> {
    let v: Vec<OneStepMarket> = inputs()
        .iter()
        .map(|m| OneStepMarket::new(m.a0, m.a1, m.s0, m.s_up, m.s_down, m.p))
        .collect();
    v.try_into().unwrap_or_else(|_| unreachable!())
}

/// Returns an operation that applies `op` to the next market of the table
/// on each call.
fn over_markets<R>(mut op: impl FnMut(&OneStepMarket) -> R) -> impl FnMut() -> R {
    let markets = markets();
    let mut i = 0;
    move || {
        let r = op(&markets[i]);
        i = (i + 1) & (NUM_MARKETS - 1);
        r
    }
}

/// Exercise 1.3: dealers A (New York) and B (London).
fn book_quotes() -> Vec<FxQuote> {
    vec![
        FxQuote::new("A", "EUR", "USD", 1.0202, 1.0284),
        FxQuote::new("A", "GBP", "USD", 1.5718, 1.5844),
        FxQuote::new("B", "EUR", "GBP", 0.6324, 0.6401),
        FxQuote::new("B", "USD", "GBP", 0.6299, 0.6375),
    ]
}

/// A complete market on `n` currencies "C0".."C<n-1>": one dealer quotes
/// every pair, around consistent mid rates with a 0.5% spread plus some
/// noise, so a few cycles are profitable.
fn synthetic_quotes(n: usize) -> Vec<FxQuote> {
    let mut rng = Rng::new(n as u64);
    let mid: Vec<f64> = (0..n).map(|_| rng.uniform(0.5, 2.0)).collect();
    let mut quotes = Vec::new();
    for i in 0..n {
        for j in i + 1..n {
            let rate = mid[i] / mid[j] * (1.0 + rng.uniform(-0.004, 0.004));
            quotes.push(FxQuote::new(
                "D",
                &format!("C{i}"),
                &format!("C{j}"),
                rate * 0.9975,
                rate * 1.0025,
            ));
        }
    }
    quotes
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // One-step market (m4f_ch01::one_step) and forwards.
    r.bench("Construct", || {
        let inputs = inputs();
        let mut i = 0;
        move || {
            let m = &inputs[i];
            i = (i + 1) & (NUM_MARKETS - 1);
            OneStepMarket::new(m.a0, m.a1, m.s0, m.s_up, m.s_down, m.p)
        }
    });
    r.bench("IsArbitrageFree", || {
        over_markets(|m| m.is_arbitrage_free())
    });
    r.bench("Arbitrage", || over_markets(|m| m.arbitrage()));
    r.bench("RiskNeutralProb", || {
        over_markets(|m| m.risk_neutral_prob())
    });
    r.bench("Replicate", || {
        over_markets(|m| {
            m.replicate(
                call_payoff(m.s1(State::Up), 100.0),
                call_payoff(m.s1(State::Down), 100.0),
            )
        })
    });
    // Call price two ways: replicating portfolio vs. risk-neutral expectation.
    r.bench("PriceByReplication", || {
        over_markets(|m| {
            m.price(
                call_payoff(m.s1(State::Up), 100.0),
                call_payoff(m.s1(State::Down), 100.0),
            )
        })
    });
    r.bench("RiskNeutralPrice", || {
        over_markets(|m| {
            m.risk_neutral_price(
                call_payoff(m.s1(State::Up), 100.0),
                call_payoff(m.s1(State::Down), 100.0),
            )
        })
    });
    // Expected return and risk of a 50/50 portfolio (Section 1.4).
    r.bench("PortfolioRisk", || {
        over_markets(|m| {
            let portfolio = m.split(10000.0, 0.5);
            let k_up = m.portfolio_return(&portfolio, State::Up);
            let k_down = m.portfolio_return(&portfolio, State::Down);
            m.expect(k_up, k_down) + m.std_dev(k_up, k_down)
        })
    });
    r.bench("HedgedPurchaseCost", || {
        over_markets(|m| hedged_purchase_cost(m, 100.0, 2.0, State::Up))
    });
    r.bench("ForwardArbitrage", || {
        let inputs = inputs();
        let mut i = 0;
        move || {
            let m = &inputs[i];
            i = (i + 1) & (NUM_MARKETS - 1);
            // Quote the forward at S^u(1): mispriced in every generated market.
            forward_arbitrage(m.s0, m.a0, m.a1, m.s_up)
        }
    });

    // Currency-cycle search (m4f_ch01::fx_arbitrage).
    r.bench("FxMarketConstruct", || {
        let quotes = book_quotes();
        move || FxMarket::new(black_box(&quotes))
    });
    r.bench("BestLeg", || {
        let market = FxMarket::new(&book_quotes());
        move || black_box(market.best_leg(black_box("USD"), black_box("GBP"))).is_some()
    });
    // The whole of Exercise 1.3.
    r.bench("BestCycleBook", || {
        let market = FxMarket::new(&book_quotes());
        move || black_box(market.best_cycle(black_box("USD"))).legs.len()
    });
    for n in 3..=8 {
        r.bench(&format!("BestCycleScaling/{n}"), || {
            let market = FxMarket::new(&synthetic_quotes(n));
            move || black_box(market.best_cycle(black_box("C0"))).legs.len()
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
