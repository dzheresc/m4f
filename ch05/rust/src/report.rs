//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::portfolio::{
    Mat, beta, capital_market_line, covariance_matrix, market_portfolio, min_variance_line,
    min_variance_portfolio, portfolio_mean, portfolio_std, security_market_line,
};
use crate::risk::{
    correlation, covariance, mean, min_variance_weight, std_dev, two_asset_mean, two_asset_std,
};

/// Four decimals, round-half-even on the exact double, like `{:.4f}` in
/// C++ `std::format`. Adding 0.0 turns -0.0 into 0.0 so that no "-0.0000"
/// appears.
struct F4(f64);

impl Display for F4 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:.4}", self.0 + 0.0)
    }
}

/// "[a, b, c]" with each entry formatted by `F4`.
struct F4List<'a>(&'a [f64]);

impl Display for F4List<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("[")?;
        for (i, &v) in self.0.iter().enumerate() {
            if i > 0 {
                f.write_str(", ")?;
            }
            write!(f, "{}", F4(v))?;
        }
        f.write_str("]")
    }
}

/// The i-th unit vector of length n.
fn unit(i: usize, n: usize) -> Vec<f64> {
    let mut e = vec![0.0; n];
    e[i] = 1.0;
    e
}

// The three-security market shared by sections 5.3 and 5.4.
const MEANS: [f64; 3] = [0.10, 0.15, 0.20];
const WEIGHTS: [f64; 3] = [0.4, 0.3, 0.3];
const RISK_FREE: f64 = 0.05;

fn market_covariance() -> Mat {
    covariance_matrix(
        &[0.15, 0.25, 0.30],
        &[
            vec![1.0, 0.3, 0.1],
            vec![0.3, 1.0, 0.4],
            vec![0.1, 0.4, 1.0],
        ],
    )
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 5: Portfolio Management (illustrative numbers, book formulas)"
    )?;
    let c = market_covariance();
    print_risk(os)?;
    print_two_securities(os)?;
    print_several_securities(os, &c)?;
    print_efficient_frontier(os, &c)?;
    print_capm(os, &c)
}

fn print_risk(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 5.1 Risk: scenarios with p = {{0.2, 0.5, 0.3}} ==")?;
    let p = [0.2, 0.5, 0.3];
    let k1 = [-0.10, 0.10, 0.20];
    let k2 = [0.20, 0.05, -0.05];
    writeln!(
        os,
        "K1={}: mu={} sigma={}",
        F4List(&k1),
        F4(mean(&k1, &p)),
        F4(std_dev(&k1, &p))
    )?;
    writeln!(
        os,
        "K2={}: mu={} sigma={}",
        F4List(&k2),
        F4(mean(&k2, &p)),
        F4(std_dev(&k2, &p))
    )?;
    writeln!(
        os,
        "Cov(K1,K2)={} rho={}",
        F4(covariance(&k1, &k2, &p)),
        F4(correlation(&k1, &k2, &p))
    )?;
    let k_v: Vec<f64> = k1.iter().zip(&k2).map(|(a, b)| 0.5 * a + 0.5 * b).collect();
    let rho = correlation(&k1, &k2, &p);
    writeln!(
        os,
        "Half and half: sigma from scenarios={}, from formula={}",
        F4(std_dev(&k_v, &p)),
        F4(two_asset_std(0.5, std_dev(&k1, &p), std_dev(&k2, &p), rho))
    )
}

fn print_two_securities(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 sigma2=0.20 =="
    )?;
    for rho in [-1.0, -0.5, 0.0, 0.5, 1.0] {
        let w = min_variance_weight(0.10, 0.20, rho);
        writeln!(
            os,
            "rho={}: min variance w1={} mu={} sigma={}; w1=0.4: sigma={}",
            F4(rho),
            F4(w),
            F4(two_asset_mean(w, 0.08, 0.12)),
            F4(two_asset_std(w, 0.10, 0.20, rho)),
            F4(two_asset_std(0.4, 0.10, 0.20, rho))
        )?;
    }
    Ok(())
}

fn print_several_securities(os: &mut impl Write, c: &Mat) -> fmt::Result {
    writeln!(
        os,
        "\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) sigma=(0.15, 0.25, 0.30) =="
    )?;
    writeln!(os, "rho12=0.3 rho13=0.1 rho23=0.4")?;
    writeln!(
        os,
        "w={}: mu={} sigma={}",
        F4List(&WEIGHTS),
        F4(portfolio_mean(&WEIGHTS, &MEANS)),
        F4(portfolio_std(&WEIGHTS, c))
    )
}

fn print_efficient_frontier(os: &mut impl Write, c: &Mat) -> fmt::Result {
    writeln!(os, "\n== 5.3.2 Efficient frontier ==")?;
    let w_min = min_variance_portfolio(c);
    writeln!(
        os,
        "Minimum variance portfolio w={} mu={} sigma={}",
        F4List(&w_min),
        F4(portfolio_mean(&w_min, &MEANS)),
        F4(portfolio_std(&w_min, c))
    )?;
    for mu in [0.15, 0.18, 0.21] {
        let w = min_variance_line(&MEANS, c, mu);
        writeln!(
            os,
            "mu_V={}: w={} sigma={}",
            F4(mu),
            F4List(&w),
            F4(portfolio_std(&w, c))
        )?;
    }
    Ok(())
}

fn print_capm(os: &mut impl Write, c: &Mat) -> fmt::Result {
    writeln!(os, "\n== 5.4.1 Capital market line: R=0.05 ==")?;
    let w_market = market_portfolio(&MEANS, c, RISK_FREE);
    let mu_m = portfolio_mean(&w_market, &MEANS);
    let sigma_m = portfolio_std(&w_market, c);
    writeln!(
        os,
        "Market portfolio w_M={} mu_M={} sigma_M={} slope={}",
        F4List(&w_market),
        F4(mu_m),
        F4(sigma_m),
        F4((mu_m - RISK_FREE) / sigma_m)
    )?;
    writeln!(
        os,
        "On the CML at sigma=0.15: mu={}",
        F4(capital_market_line(0.15, RISK_FREE, mu_m, sigma_m))
    )?;
    writeln!(
        os,
        "\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 =="
    )?;
    let betas: Vec<f64> = (0..3).map(|i| beta(&unit(i, 3), &w_market, c)).collect();
    writeln!(os, "beta={}", F4List(&betas))?;
    let mut weighted_beta = 0.0;
    for i in 0..3 {
        weighted_beta += WEIGHTS[i] * betas[i];
    }
    writeln!(
        os,
        "w={}: beta_V={} sum w_i beta_i={}",
        F4List(&WEIGHTS),
        F4(beta(&WEIGHTS, &w_market, c)),
        F4(weighted_beta)
    )?;
    writeln!(
        os,
        "\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V =="
    )?;
    for i in 0..3 {
        writeln!(
            os,
            "security {}: beta={} SML mu={} actual mu={}",
            i + 1,
            F4(betas[i]),
            F4(security_market_line(betas[i], RISK_FREE, mu_m)),
            F4(MEANS[i])
        )?;
    }
    Ok(())
}
