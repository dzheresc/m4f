//! Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k)
//! after k up moves in n steps, with the money market growing at r per step.

use crate::libm::lgamma;

/// C(n, k) p^k (1-p)^(n-k), computed in log space: C(n, k) alone passes
/// `f64::MAX` near n = 1030 even when the probability itself is tiny.
pub fn binomial_prob(n: i32, k: i32, p: f64) -> f64 {
    if k < 0 || k > n {
        return 0.0;
    }
    if p <= 0.0 {
        return if k == 0 { 1.0 } else { 0.0 };
    }
    if p >= 1.0 {
        return if k == n { 1.0 } else { 0.0 };
    }
    let (nf, kf) = (f64::from(n), f64::from(k));
    let log_c = lgamma(nf + 1.0) - lgamma(kf + 1.0) - lgamma(nf - kf + 1.0);
    (log_c + kf * p.ln() + (nf - kf) * (-p).ln_1p()).exp()
}

/// A recombining binomial tree.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BinomialTree {
    /// S(0).
    pub s0: f64,
    /// Up return u.
    pub u: f64,
    /// Down return d.
    pub d: f64,
    /// Risk-free return r per step.
    pub r: f64,
}

impl BinomialTree {
    /// S(n) after k up moves out of n.
    pub fn price(&self, n: i32, k: i32) -> f64 {
        self.s0 * (1.0 + self.u).powf(f64::from(k)) * (1.0 + self.d).powf(f64::from(n - k))
    }

    /// E(K(n)) = p u + (1-p) d, the same for every step.
    pub fn expected_return(&self, p: f64) -> f64 {
        p * self.u + (1.0 - p) * self.d
    }

    /// E(S(n)) as a sum over the n+1 nodes at step n, O(n). Each term
    /// C(n, k) p^k (1-p)^(n-k) S(n, k) is built in log space, updating
    /// log C(n, k) from k to k+1, so nothing overflows for large n.
    pub fn expected_price(&self, n: i32, p: f64) -> f64 {
        if p <= 0.0 {
            return self.price(n, 0);
        }
        if p >= 1.0 {
            return self.price(n, n);
        }
        let log_up = p.ln() + self.u.ln_1p();
        let log_down = (-p).ln_1p() + self.d.ln_1p();
        let mut log_c = 0.0;
        let mut sum = 0.0;
        for k in 0..n + 1 {
            sum += (log_c + f64::from(k) * log_up + f64::from(n - k) * log_down).exp();
            if k < n {
                log_c += f64::from(n - k).ln() - (f64::from(k) + 1.0).ln();
            }
        }
        self.s0 * sum
    }

    /// 3.2.1 Risk-neutral probability exists: d < r < u.
    pub fn is_arbitrage_free(&self) -> bool {
        self.d < self.r && self.r < self.u
    }

    /// p* = (r - d) / (u - d), so that E*(K(n)) = r.
    pub fn risk_neutral_prob(&self) -> f64 {
        (self.r - self.d) / (self.u - self.d)
    }

    /// 3.2.2 Martingale property: S~(n) = (1+r)^-n S(n).
    pub fn discounted_price(&self, n: i32, k: i32) -> f64 {
        self.price(n, k) * (1.0 + self.r).powf(f64::from(-n))
    }

    /// E*(S~(n+1) | S(n) at node (n, k)); equals S~(n) under p*.
    pub fn conditional_discounted(&self, n: i32, k: i32) -> f64 {
        let q = self.risk_neutral_prob();
        q * self.discounted_price(n + 1, k + 1) + (1.0 - q) * self.discounted_price(n + 1, k)
    }
}
