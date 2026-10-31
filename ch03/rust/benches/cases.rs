//! Benchmark cases for chapter 3, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch03::binomial_tree::{BinomialTree, binomial_prob};
use m4f_ch03::other_models::{
    TrinomialModel, binomial_expected_price, binomial_prob_above, lognormal_expected_price,
    lognormal_prob_above, norm_cdf,
};
use m4f_ch03::report::print_report;
use m4f_ch03::returns::{compound_return, expectation, log_returns, returns, variance};

/// Random arbitrage-free trees: r in [0, 5%], u and d 1% - 20% either side.
fn trees() -> Vec<BinomialTree> {
    let mut rng = Rng::new(20261031);
    table(|| {
        let r = rng.uniform(0.0, 0.05);
        let u = r + rng.uniform(0.01, 0.2);
        let d = r - rng.uniform(0.01, 0.2);
        BinomialTree { s0: 100.0, u, d, r }
    })
}

fn uniform(lo: f64, hi: f64, seed: u64) -> Vec<f64> {
    let mut rng = Rng::new(seed);
    table(|| rng.uniform(lo, hi))
}

/// A daily price path of `days` steps with log returns N(0.0003, 0.01).
fn path(days: usize) -> Vec<f64> {
    let mut rng = Rng::new(days as u64);
    let mut path = vec![100.0];
    for _ in 0..days {
        let last = *path.last().expect("path is not empty");
        path.push(last * rng.normal(0.0003, 0.01).exp());
    }
    path
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Section 3.1 (m4f_ch03::returns).
    for days in [252, 2520] {
        r.bench(&format!("Returns/{days}"), || {
            let path = path(days);
            move || returns(&path)
        });
    }
    for days in [252, 2520] {
        r.bench(&format!("LogReturns/{days}"), || {
            let path = path(days);
            move || log_returns(&path)
        });
    }
    for days in [252, 2520] {
        r.bench(&format!("CompoundReturn/{days}"), || {
            let returns = returns(&path(days));
            move || compound_return(std::hint::black_box(&returns))
        });
    }
    for days in [252, 2520] {
        r.bench(&format!("ExpectationVariance/{days}"), || {
            let returns = returns(&path(days));
            let probs = vec![1.0 / returns.len() as f64; returns.len()];
            move || {
                let returns = std::hint::black_box(&returns);
                expectation(returns, &probs) + variance(returns, &probs)
            }
        });
    }

    // Section 3.2 (m4f_ch03::binomial_tree).
    for n in [3, 10, 100, 1000] {
        r.bench(&format!("BinomialProb/{n}"), || {
            cycle(trees(), move |t| {
                binomial_prob(n, n / 2, t.risk_neutral_prob())
            })
        });
    }
    r.bench("Price", || cycle(trees(), |t| t.price(10, 4)));
    r.bench("RiskNeutralProb", || {
        cycle(trees(), |t| t.risk_neutral_prob())
    });
    for n in [3, 10, 100, 1000] {
        r.bench(&format!("ExpectedPrice/{n}"), || {
            cycle(trees(), move |t| t.expected_price(n, 0.6))
        });
    }
    r.bench("ConditionalDiscounted", || {
        cycle(trees(), |t| t.conditional_discounted(10, 4))
    });

    // Section 3.3 (m4f_ch03::other_models).
    r.bench("TrinomialRiskNeutralProbs", || {
        let model = TrinomialModel {
            u: 0.1,
            m: 0.0,
            d: -0.1,
            r: 0.02,
        };
        cycle(uniform(0.0, model.max_middle_prob(), 1), move |&p| {
            model.risk_neutral_probs(p)
        })
    });
    r.bench("NormCdf", || cycle(uniform(-4.0, 4.0, 2), |&x| norm_cdf(x)));
    for n in [10, 100, 1000, 10000] {
        r.bench(&format!("BinomialExpectedPrice/{n}"), || {
            cycle(uniform(0.1, 0.4, 3), move |&sigma| {
                binomial_expected_price(100.0, 0.05, sigma, 1.0, n)
            })
        });
    }
    r.bench("LognormalExpectedPrice", || {
        cycle(uniform(0.1, 0.4, 3), |&sigma| {
            lognormal_expected_price(100.0, 0.05, sigma, 1.0)
        })
    });
    for n in [10, 100, 1000, 10000] {
        r.bench(&format!("BinomialProbAbove/{n}"), || {
            cycle(uniform(0.1, 0.4, 3), move |&sigma| {
                binomial_prob_above(100.0, 0.05, sigma, 1.0, n, 110.0)
            })
        });
    }
    r.bench("LognormalProbAbove", || {
        cycle(uniform(0.1, 0.4, 3), |&sigma| {
            lognormal_prob_above(100.0, 0.05, sigma, 1.0, 110.0)
        })
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
