//! Section 3.3, other models: the trinomial tree and the continuous-time
//! limit of the binomial model (log returns m tau +/- sigma sqrt(tau) with
//! probability 1/2).
//!
//! Book notation: S(0) is `s0`, T the horizon (`horizon`), N the number of
//! steps (`num_steps`), K a threshold price (`threshold`), m the drift
//! (`mu`).

/// Risk-neutral probabilities of the three moves.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RiskNeutralProbs {
    /// p_u.
    pub p_up: f64,
    /// p_m.
    pub p_mid: f64,
    /// p_d.
    pub p_down: f64,
}

/// 3.3.1 Trinomial tree model: returns u > m > d, money market r.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct TrinomialModel {
    /// Up return u per step.
    pub u: f64,
    /// Middle return m per step.
    pub m: f64,
    /// Down return d per step.
    pub d: f64,
    /// Money market return r per step.
    pub r: f64,
}

impl TrinomialModel {
    /// (p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the
    /// risk-neutral probability is not unique in this model.
    pub fn risk_neutral_probs(&self, p_mid: f64) -> RiskNeutralProbs {
        let p_up = (self.r - p_mid * self.m - (1.0 - p_mid) * self.d) / (self.u - self.d);
        RiskNeutralProbs {
            p_up,
            p_mid,
            p_down: 1.0 - p_mid - p_up,
        }
    }

    /// p_u, p_d > 0 exactly when 0 < p_m < this bound.
    pub fn max_middle_prob(&self) -> f64 {
        ((self.r - self.d) / (self.m - self.d)).min((self.u - self.r) / (self.u - self.m))
    }
}

/// 3.3.2 Standard normal CDF, Abramowitz & Stegun 26.2.17
/// (|error| < 7.5e-8).
pub fn norm_cdf(x: f64) -> f64 {
    if x < 0.0 {
        return 1.0 - norm_cdf(-x);
    }
    let t = 1.0 / (1.0 + 0.2316419 * x);
    let poly = t
        * (0.319381530
            + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))));
    1.0 - (-x * x / 2.0).exp() / (2.0 * std::f64::consts::PI).sqrt() * poly
}

/// E(S(T)) with N steps of length tau = T/N.
pub fn binomial_expected_price(s0: f64, mu: f64, sigma: f64, horizon: f64, num_steps: i32) -> f64 {
    let tau = horizon / f64::from(num_steps);
    let a = mu * tau;
    let b = sigma * tau.sqrt();
    s0 * (0.5 * (a + b).exp() + 0.5 * (a - b).exp()).powf(f64::from(num_steps))
}

/// Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T).
pub fn lognormal_expected_price(s0: f64, mu: f64, sigma: f64, horizon: f64) -> f64 {
    s0 * ((mu + sigma * sigma / 2.0) * horizon).exp()
}

/// P(S(T) > K) in the N-step binomial model.
pub fn binomial_prob_above(
    s0: f64,
    mu: f64,
    sigma: f64,
    horizon: f64,
    num_steps: i32,
    threshold: f64,
) -> f64 {
    let b = sigma * (horizon / f64::from(num_steps)).sqrt();
    let log_threshold = (threshold / s0).ln() - mu * horizon;
    let log_half_n = f64::from(num_steps) * 0.5f64.ln();
    let mut log_c = 0.0;
    let mut total = 0.0;
    for k in 0..num_steps + 1 {
        if f64::from(2 * k - num_steps) * b > log_threshold {
            total += (log_c + log_half_n).exp();
        }
        if k < num_steps {
            log_c += f64::from(num_steps - k).ln() - f64::from(k + 1).ln();
        }
    }
    total
}

/// Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T).
pub fn lognormal_prob_above(s0: f64, mu: f64, sigma: f64, horizon: f64, threshold: f64) -> f64 {
    norm_cdf(((s0 / threshold).ln() + mu * horizon) / (sigma * horizon.sqrt()))
}
