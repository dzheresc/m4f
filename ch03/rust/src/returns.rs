//! Section 3.1, dynamics of stock prices: returns, logarithmic returns and
//! their expectations over a finite set of scenarios.
//!
//! Book notation: S(0), S(1) are `s0`, `s1`; K is a one-step return and k a
//! logarithmic return.

/// 3.1.1 Return: K(1) = (S(1) + div - S(0)) / S(0). (C++ defaults
/// `dividend` to 0.)
pub fn one_step_return(s0: f64, s1: f64, dividend: f64) -> f64 {
    (s1 + dividend - s0) / s0
}

/// Logarithmic return k(1) = ln(S(1) / S(0)).
pub fn log_return(s0: f64, s1: f64) -> f64 {
    (s1 / s0).ln()
}

/// One-step returns K(n) along a price path S(0), S(1), ...
pub fn returns(path: &[f64]) -> Vec<f64> {
    path.windows(2)
        .map(|w| one_step_return(w[0], w[1], 0.0))
        .collect()
}

/// One-step logarithmic returns k(n) along a price path.
pub fn log_returns(path: &[f64]) -> Vec<f64> {
    path.windows(2).map(|w| log_return(w[0], w[1])).collect()
}

/// K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n)).
pub fn compound_return(returns: &[f64]) -> f64 {
    returns.iter().fold(1.0, |growth, k| growth * (1.0 + k)) - 1.0
}

/// 3.1.2 Expected return: E(X) over a finite set of scenarios.
pub fn expectation(values: &[f64], probs: &[f64]) -> f64 {
    values
        .iter()
        .zip(probs)
        .fold(0.0, |sum, (v, p)| sum + v * p)
}

/// Var(X) over a finite set of scenarios.
pub fn variance(values: &[f64], probs: &[f64]) -> f64 {
    let mean = expectation(values, probs);
    values.iter().zip(probs).fold(0.0, |sum, (v, p)| {
        let deviation = v - mean;
        sum + p * deviation * deviation
    })
}
