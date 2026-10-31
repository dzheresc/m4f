//! Benchmark cases for chapter 5, mirroring the C++ Google Benchmark suite
//! (`cpp/benchmarks/*_benchmark.cc`) case for case and under the same names
//! without the `BM_` prefix.

use std::hint::black_box;

use crate::harness::{Rng, Runner, cycle, table};
use m4f_ch05::portfolio::{
    Mat, beta, covariance_matrix, inverse, market_portfolio, min_variance_line,
    min_variance_portfolio, portfolio_mean, portfolio_std, vec_mat,
};
use m4f_ch05::report::print_report;
use m4f_ch05::risk::{correlation, covariance, min_variance_weight, two_asset_std};

struct Scenarios {
    k1: Vec<f64>,
    k2: Vec<f64>,
    p: Vec<f64>,
}

fn make_scenarios(n: usize) -> Scenarios {
    let mut rng = Rng::new(n as u64);
    let mut s = Scenarios {
        k1: Vec::new(),
        k2: Vec::new(),
        p: vec![1.0 / n as f64; n],
    };
    for _ in 0..n {
        let k1 = rng.normal(0.08, 0.2);
        s.k1.push(k1);
        s.k2.push(0.5 * k1 + rng.normal(0.08, 0.2));
    }
    s
}

#[derive(Clone, Copy)]
struct TwoAssets {
    w1: f64,
    s1: f64,
    s2: f64,
    rho: f64,
}

fn pairs() -> Vec<TwoAssets> {
    let mut rng = Rng::new(20261031);
    table(|| TwoAssets {
        w1: rng.uniform(-0.5, 1.5),
        s1: rng.uniform(0.1, 0.4),
        s2: rng.uniform(0.1, 0.4),
        rho: rng.uniform(-0.9, 0.9),
    })
}

struct Market {
    m: Vec<f64>,
    sigma: Vec<f64>,
    rho: Mat,
    cov: Mat,
    /// Some fully invested portfolio.
    w: Vec<f64>,
}

/// n assets driven by two factors plus idiosyncratic variance.
fn make_market(n: usize) -> Market {
    let mut rng = Rng::new(n as u64);
    let mut m = Vec::new();
    let mut factors = Vec::new();
    for _ in 0..n {
        m.push(rng.uniform(0.02, 0.15));
        factors.push([rng.uniform(-0.2, 0.3), rng.uniform(-0.2, 0.3)]);
    }
    let mut cov = vec![vec![0.0; n]; n];
    for i in 0..n {
        for j in 0..n {
            cov[i][j] = factors[i][0] * factors[j][0] + factors[i][1] * factors[j][1];
        }
        cov[i][i] += rng.uniform(0.01, 0.04);
    }
    let sigma: Vec<f64> = (0..n).map(|i| cov[i][i].sqrt()).collect();
    let rho = (0..n)
        .map(|i| (0..n).map(|j| cov[i][j] / (sigma[i] * sigma[j])).collect())
        .collect();
    Market {
        m,
        sigma,
        rho,
        cov,
        w: vec![1.0 / n as f64; n],
    }
}

const ASSET_COUNTS: [usize; 4] = [3, 10, 50, 200];

/// Runs every case.
pub fn all(r: &mut impl Runner) {
    // Sections 5.1-5.2 (m4f_ch05::risk).
    for n in [3, 100, 10000] {
        r.bench(&format!("Covariance/{n}"), || {
            let s = make_scenarios(n);
            move || covariance(black_box(&s.k1), &s.k2, &s.p)
        });
    }
    for n in [3, 100, 10000] {
        r.bench(&format!("Correlation/{n}"), || {
            let s = make_scenarios(n);
            move || correlation(black_box(&s.k1), &s.k2, &s.p)
        });
    }
    r.bench("TwoAssetStd", || {
        cycle(pairs(), |a| two_asset_std(a.w1, a.s1, a.s2, a.rho))
    });
    r.bench("MinVarianceWeight", || {
        cycle(pairs(), |a| min_variance_weight(a.s1, a.s2, a.rho))
    });

    // Sections 5.3-5.4 (m4f_ch05::portfolio).
    macro_rules! market_case {
        ($name:literal, |$mk:ident| $expr:expr) => {
            for n in ASSET_COUNTS {
                r.bench(&format!(concat!($name, "/{}"), n), || {
                    let market = make_market(n);
                    move || {
                        let $mk = black_box(&market);
                        $expr
                    }
                });
            }
        };
    }
    market_case!("CovarianceMatrix", |mk| covariance_matrix(
        &mk.sigma, &mk.rho
    ));
    market_case!("VecMat", |mk| vec_mat(&mk.w, &mk.cov));
    market_case!("Inverse", |mk| inverse(&mk.cov));
    market_case!("PortfolioMean", |mk| portfolio_mean(&mk.w, &mk.m));
    market_case!("PortfolioStd", |mk| portfolio_std(&mk.w, &mk.cov));
    market_case!("MinVariancePortfolio", |mk| min_variance_portfolio(&mk.cov));
    market_case!("MinVarianceLine", |mk| min_variance_line(
        &mk.m, &mk.cov, 0.1
    ));
    market_case!("MarketPortfolio", |mk| market_portfolio(
        &mk.m, &mk.cov, 0.01
    ));
    market_case!("Beta", |mk| beta(&mk.w, &mk.m, &mk.cov));

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
