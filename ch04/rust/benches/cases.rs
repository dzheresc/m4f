//! Benchmark cases for chapter 4, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner};
use m4f_ch04::no_arbitrage::{
    arbitrage_strategy, expected_discounted_value, is_arbitrage_free, one_step_values,
    risk_neutral_probs,
};
use m4f_ch04::report::print_report;
use m4f_ch04::strategies::{discounted_gains, gains, self_financing, self_financing_multi};

struct Market {
    s: Vec<Vec<f64>>,
    a: Vec<f64>,
    x: Vec<Vec<f64>>,
    dividends: Vec<Vec<f64>>,
}

/// `stocks` daily price paths of `steps` steps, random positions in
/// [0, 20) and a quarterly dividend of 0.5.
fn make_market(stocks: usize, steps: usize) -> Market {
    let mut rng = Rng::new((stocks * 100003 + steps) as u64);
    let mut m = Market {
        s: Vec::new(),
        a: (0..=steps).map(|n| 1.0001f64.powf(n as f64)).collect(),
        x: Vec::new(),
        dividends: Vec::new(),
    };
    for _ in 0..stocks {
        let mut s = vec![100.0];
        let mut x = Vec::new();
        let mut div = vec![0.0; steps + 1];
        for n in 1..=steps {
            s.push(s[n - 1] * rng.normal(0.0003, 0.01).exp());
            x.push(rng.uniform(0.0, 20.0));
            if n % 63 == 0 {
                div[n] = 0.5;
            }
        }
        m.s.push(s);
        m.x.push(x);
        m.dividends.push(div);
    }
    m
}

fn scenarios(n: usize) -> Vec<f64> {
    let mut rng = Rng::new(n as u64);
    (0..n).map(|_| rng.uniform(-0.2, 0.25)).collect()
}

fn contrarian(_n: i32, moves: &[i32]) -> f64 {
    let mut downs = 0.0;
    for &m in moves {
        downs += if m == 0 { 1.0 } else { 0.0 };
    }
    1.0 + downs
}

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Section 4.1.1 and 4.2 (m4f_ch04::strategies).
    for steps in [12, 252, 2520] {
        r.bench(&format!("SelfFinancing/{steps}"), || {
            let m = make_market(1, steps);
            move || self_financing(100.0, black_box(&m.s[0]), &m.a, &m.x[0])
        });
    }
    for steps in [12, 252, 2520] {
        r.bench(&format!("SelfFinancingMulti/{steps}"), || {
            let m = make_market(5, steps);
            move || self_financing_multi(1000.0, black_box(&m.s), &m.a, &m.x, Some(&m.dividends))
        });
    }
    for steps in [12, 252, 2520] {
        r.bench(&format!("Gains/{steps}"), || {
            let m = make_market(1, steps);
            let y = self_financing(100.0, &m.s[0], &m.a, &m.x[0]).y;
            move || gains(black_box(&m.s[0]), &m.a, &m.x[0], &y)
        });
    }
    for steps in [12, 252, 2520] {
        r.bench(&format!("DiscountedGains/{steps}"), || {
            let m = make_market(1, steps);
            move || discounted_gains(black_box(&m.s[0]), &m.a, &m.x[0])
        });
    }

    // Sections 4.1.2-4.1.4 (m4f_ch04::no_arbitrage).
    for n in [2, 3, 100] {
        r.bench(&format!("IsArbitrageFree/{n}"), || {
            let returns = scenarios(n);
            move || is_arbitrage_free(black_box(&returns), 0.02)
        });
    }
    for n in [2, 3, 100] {
        r.bench(&format!("ArbitrageStrategy/{n}"), || {
            let returns = scenarios(n);
            move || arbitrage_strategy(black_box(&returns), 0.3, 100.0, 100.0)
        });
    }
    for n in [2, 3, 100] {
        r.bench(&format!("RiskNeutralProbs/{n}"), || {
            let returns = scenarios(n);
            move || risk_neutral_probs(black_box(&returns), 0.02)
        });
    }
    for n in [2, 3, 100] {
        r.bench(&format!("OneStepValues/{n}"), || {
            let returns = scenarios(n);
            move || one_step_values(-1.0, 1.0, black_box(&returns), 0.02, 100.0, 100.0)
        });
    }
    for num_steps in [4, 8, 12, 16] {
        r.bench(&format!("ExpectedDiscountedValue/{num_steps}"), || {
            let q = (0.03 - -0.05) / (0.10 - -0.05);
            move || {
                expected_discounted_value(
                    100.0,
                    0.10,
                    -0.05,
                    0.03,
                    black_box(num_steps),
                    100.0,
                    &contrarian,
                    q,
                )
            }
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
