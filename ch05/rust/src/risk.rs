//! Sections 5.1-5.2: risk as the standard deviation of return, and
//! portfolios of two securities with weights w1 + w2 = 1.
//!
//! Book notation: k (or k1, k2) are scenario returns, p their
//! probabilities, mu an expected return, s1, s2 (sigma) standard
//! deviations and rho a correlation.

/// Section 5.1: expected return from returns k_i in scenarios with
/// probabilities p_i.
pub fn mean(k: &[f64], p: &[f64]) -> f64 {
    let mut s = 0.0;
    for (k_i, p_i) in k.iter().zip(p) {
        s += k_i * p_i;
    }
    s
}

/// Cov(K1, K2).
pub fn covariance(k1: &[f64], k2: &[f64], p: &[f64]) -> f64 {
    let m1 = mean(k1, p);
    let m2 = mean(k2, p);
    // Same order as the C++ loop; zip leaves no bounds checks in it.
    let mut s = 0.0;
    for ((p_i, a), b) in p.iter().zip(k1).zip(k2) {
        s += p_i * (a - m1) * (b - m2);
    }
    s
}

/// Standard deviation sigma(K).
pub fn std_dev(k: &[f64], p: &[f64]) -> f64 {
    covariance(k, k, p).sqrt()
}

/// Correlation rho(K1, K2). One pass for the three second moments; same
/// arithmetic as `covariance(k1, k2, p) / (std_dev(k1, p) * std_dev(k2, p))`.
pub fn correlation(k1: &[f64], k2: &[f64], p: &[f64]) -> f64 {
    let m1 = mean(k1, p);
    let m2 = mean(k2, p);
    let mut cov = 0.0;
    let mut var1 = 0.0;
    let mut var2 = 0.0;
    for i in 0..k1.len() {
        cov += p[i] * (k1[i] - m1) * (k2[i] - m2);
        var1 += p[i] * (k1[i] - m1) * (k1[i] - m1);
        var2 += p[i] * (k2[i] - m2) * (k2[i] - m2);
    }
    cov / (var1.sqrt() * var2.sqrt())
}

/// Section 5.2.1: two securities, w1 in the first and w2 = 1 - w1 in the
/// second. mu_V = w1 mu1 + w2 mu2.
pub fn two_asset_mean(w1: f64, mu1: f64, mu2: f64) -> f64 {
    w1 * mu1 + (1.0 - w1) * mu2
}

/// sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0
/// against rounding when |rho| = 1 makes the portfolio riskless.
pub fn two_asset_std(w1: f64, s1: f64, s2: f64, rho: f64) -> f64 {
    let w2 = 1.0 - w1;
    0.0f64
        .max(w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2.0 * w1 * w2 * rho * s1 * s2)
        .sqrt()
}

/// w1 of the minimum variance portfolio:
/// (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2).
pub fn min_variance_weight(s1: f64, s2: f64, rho: f64) -> f64 {
    (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2.0 * rho * s1 * s2)
}
