"""Section 3.3, other models: the trinomial tree and the continuous-time limit
of the binomial model (log returns m tau +/- sigma sqrt(tau) with probability 1/2).
"""
from __future__ import annotations

import math


# 3.3.1 Trinomial tree model: returns u > m > d, money market r.
class TrinomialModel:
    def __init__(self, u: float, m: float, d: float, r: float) -> None:
        self.u, self.m, self.d, self.r = u, m, d, r

    def risk_neutral_probs(self, pm: float) -> tuple[float, float, float]:
        """(p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m; the
        risk-neutral probability is not unique in this model."""
        pu = (self.r - pm * self.m - (1 - pm) * self.d) / (self.u - self.d)
        return pu, pm, 1 - pm - pu

    def max_middle_prob(self) -> float:
        """p_u, p_d > 0 exactly when 0 < p_m < this bound."""
        return min((self.r - self.d) / (self.m - self.d), (self.u - self.r) / (self.u - self.m))


# 3.3.2 Continuous-time limit
def norm_cdf(x: float) -> float:
    """Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8)."""
    if x < 0:
        return 1 - norm_cdf(-x)
    t = 1 / (1 + 0.2316419 * x)
    poly = t * (0.319381530 + t * (-0.356563782 + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))))
    return 1 - math.exp(-x * x / 2) / math.sqrt(2 * math.pi) * poly


def binomial_expected_price(S0: float, mu: float, sigma: float, T: float, N: int) -> float:
    """E(S(T)) with N steps of length tau = T/N."""
    tau = T / N
    a, b = mu * tau, sigma * math.sqrt(tau)
    return S0 * (0.5 * math.exp(a + b) + 0.5 * math.exp(a - b)) ** N


def lognormal_expected_price(S0: float, mu: float, sigma: float, T: float) -> float:
    """Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T)."""
    return S0 * math.exp((mu + sigma * sigma / 2) * T)


def binomial_prob_above(S0: float, mu: float, sigma: float, T: float, N: int, K: float) -> float:
    """P(S(T) > K) in the N-step binomial model."""
    b = sigma * math.sqrt(T / N)
    threshold = math.log(K / S0) - mu * T
    log_half_n = N * math.log(0.5)
    log_c = 0.0
    total = 0.0
    for k in range(N + 1):
        if (2 * k - N) * b > threshold:
            total += math.exp(log_c + log_half_n)
        if k < N:
            log_c += math.log(N - k) - math.log(k + 1)
    return total


def lognormal_prob_above(S0: float, mu: float, sigma: float, T: float, K: float) -> float:
    """Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T)."""
    return norm_cdf((math.log(S0 / K) + mu * T) / (sigma * math.sqrt(T)))
