//! Black-Scholes prices and the Greek parameters (Section 9.1.2).
//!
//! `s` is the stock price, `strike` the strike X, `r` the continuously
//! compounded rate, `sigma` the volatility and `maturity` the time T to
//! expiry. Same `norm_cdf` as Chapter 8 (Hart 1968 / G. West).
#![allow(clippy::excessive_precision)]

use crate::max;

/// Standard normal CDF to double precision.
#[inline]
pub fn norm_cdf(x: f64) -> f64 {
    let a = x.abs();
    let c = if a > 37.0 {
        0.0
    } else {
        let e = (-a * a / 2.0).exp();
        if a < 7.07106781186547 {
            let mut b = 3.52624965998911e-02 * a + 0.700383064443688;
            b = b * a + 6.37396220353165;
            b = b * a + 33.912866078383;
            b = b * a + 112.079291497871;
            b = b * a + 221.213596169931;
            b = b * a + 220.206867912376;
            let c = e * b;
            b = 8.83883476483184e-02 * a + 1.75566716318264;
            b = b * a + 16.064177579207;
            b = b * a + 86.7807322029461;
            b = b * a + 296.564248779674;
            b = b * a + 637.333633378831;
            b = b * a + 793.826512519948;
            b = b * a + 440.413735824752;
            c / b
        } else {
            let mut b = a + 0.65;
            b = a + 4.0 / b;
            b = a + 3.0 / b;
            b = a + 2.0 / b;
            b = a + 1.0 / b;
            e / b / 2.506628274631
        }
    };
    if x > 0.0 { 1.0 - c } else { c }
}

/// Standard normal density.
#[inline]
pub fn norm_pdf(x: f64) -> f64 {
    (-x * x / 2.0).exp() / 2.506628274631000502
}

/// Inverse of `norm_cdf`. Acklam's rational approximation (relative error
/// below 1.2e-9) followed by one Halley step against `norm_cdf` itself,
/// which brings it to full double precision, about the cost of two
/// `norm_cdf` calls. `p` outside (0, 1) returns -40 or 40.
pub fn norm_inv(p: f64) -> f64 {
    if p.is_nan() || p <= 0.0 {
        return -40.0;
    }
    if p >= 1.0 {
        return 40.0;
    }
    const A: [f64; 6] = [
        -3.969683028665376e+01,
        2.209460984245205e+02,
        -2.759285104469687e+02,
        1.383577518672690e+02,
        -3.066479806614716e+01,
        2.506628277459239e+00,
    ];
    const B: [f64; 5] = [
        -5.447609879822406e+01,
        1.615858368580409e+02,
        -1.556989798598866e+02,
        6.680131188771972e+01,
        -1.328068155288572e+01,
    ];
    const C: [f64; 6] = [
        -7.784894002430293e-03,
        -3.223964580411365e-01,
        -2.400758277161838e+00,
        -2.549732539343734e+00,
        4.374664141464968e+00,
        2.938163982698783e+00,
    ];
    const D: [f64; 4] = [
        7.784695709041462e-03,
        3.224671290700398e-01,
        2.445134137142996e+00,
        3.754408661907416e+00,
    ];
    const LOW: f64 = 0.02425;
    let x = if !(LOW..=1.0 - LOW).contains(&p) {
        // Tails: q = sqrt(-2 ln(min(p, 1-p))).
        let q = (-2.0 * (if p < LOW { p } else { 1.0 - p }).ln()).sqrt();
        let x = (((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q + C[4]) * q + C[5])
            / ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q + 1.0);
        if p > 1.0 - LOW { -x } else { x }
    } else {
        let q = p - 0.5;
        let r = q * q;
        (((((A[0] * r + A[1]) * r + A[2]) * r + A[3]) * r + A[4]) * r + A[5]) * q
            / (((((B[0] * r + B[1]) * r + B[2]) * r + B[3]) * r + B[4]) * r + 1.0)
    };
    // Halley step on norm_cdf(x) - p.
    let e = norm_cdf(x) - p;
    let u = e * 2.506628274631000502 * (x * x / 2.0).exp();
    x - u / (1.0 + x * u / 2.0)
}

/// (d1, d2) with d1 = (ln(S/X) + (r + sigma^2/2) T) / (sigma sqrt(T)) and
/// d2 = d1 - sigma sqrt(T).
#[inline]
pub fn d1_d2(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64) -> (f64, f64) {
    let d1 = ((s / strike).ln() + (r + sigma * sigma / 2.0) * maturity) / (sigma * maturity.sqrt());
    (d1, d1 - sigma * maturity.sqrt())
}

/// Payoff of a call or put at stock price `s`.
#[inline]
pub fn payoff(s: f64, strike: f64, is_call: bool) -> f64 {
    if is_call {
        max(s - strike, 0.0)
    } else {
        max(strike - s, 0.0)
    }
}

/// Black-Scholes price; the payoff when `maturity <= 0`.
#[inline]
pub fn bs_price(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64, is_call: bool) -> f64 {
    if maturity <= 0.0 {
        return payoff(s, strike, is_call);
    }
    let (d1, d2) = d1_d2(s, strike, r, sigma, maturity);
    if is_call {
        return s * norm_cdf(d1) - strike * (-r * maturity).exp() * norm_cdf(d2);
    }
    strike * (-r * maturity).exp() * norm_cdf(-d2) - s * norm_cdf(-d1)
}

/// The price and Greek parameters of an option.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Greeks {
    /// V.
    pub price: f64,
    /// dV/dS.
    pub delta: f64,
    /// d2V/dS2.
    pub gamma: f64,
    /// dV/dt (per year).
    pub theta: f64,
    /// dV/dsigma.
    pub vega: f64,
    /// dV/dr.
    pub rho: f64,
}

/// Names one of the Greek parameters.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Greek {
    /// dV/dS.
    Delta,
    /// d2V/dS2.
    Gamma,
    /// dV/dt.
    Theta,
    /// dV/dsigma.
    Vega,
    /// dV/dr.
    Rho,
}

/// The Greek `which` of `g`.
#[inline]
pub fn get_greek(g: &Greeks, which: Greek) -> f64 {
    match which {
        Greek::Delta => g.delta,
        Greek::Gamma => g.gamma,
        Greek::Theta => g.theta,
        Greek::Vega => g.vega,
        Greek::Rho => g.rho,
    }
}

/// The Greek parameters of a European option in the Black-Scholes model.
/// Always inlined, like the C++ (GCC inlines it at every call site): a
/// caller that wants one Greek then skips the `norm_cdf` calls the others
/// need. LLVM does not inline it on its own.
#[inline(always)]
pub fn bs_greeks(s: f64, strike: f64, r: f64, sigma: f64, maturity: f64, is_call: bool) -> Greeks {
    let (d1, d2) = d1_d2(s, strike, r, sigma, maturity);
    let n1 = norm_pdf(d1);
    let disc = strike * (-r * maturity).exp();
    let gamma = n1 / (s * sigma * maturity.sqrt());
    let vega = s * n1 * maturity.sqrt();
    // Each norm_cdf is evaluated once, as in the C++.
    let theta0 = -s * n1 * sigma / (2.0 * maturity.sqrt());
    if is_call {
        let nd1 = norm_cdf(d1);
        let nd2 = norm_cdf(d2);
        return Greeks {
            price: s * nd1 - disc * nd2,
            delta: nd1,
            gamma,
            theta: theta0 - r * disc * nd2,
            vega,
            rho: maturity * disc * nd2,
        };
    }
    let nmd1 = norm_cdf(-d1);
    let nmd2 = norm_cdf(-d2);
    Greeks {
        price: disc * nmd2 - s * nmd1,
        delta: norm_cdf(d1) - 1.0,
        gamma,
        theta: theta0 + r * disc * nmd2,
        vega,
        rho: -maturity * disc * nmd2,
    }
}
