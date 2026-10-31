"""Section 3.3, other models.

The trinomial tree and the continuous-time limit of the binomial model (log
returns m tau +/- sigma sqrt(tau) with probability 1/2). Book notation: S(0)
is `s0`, T the horizon (`horizon`), N the number of steps (`num_steps`), K a
threshold price (`threshold`).
"""

from __future__ import annotations

import math


# 3.3.1 Trinomial tree model: returns u > m > d, money market r.
class TrinomialModel:
    """Returns u > m > d per step and money market rate r."""

    def __init__(self, u: float, m: float, d: float, r: float) -> None:
        self.u = u
        self.m = m
        self.d = d
        self.r = r

    def risk_neutral_probs(self, p_mid: float) -> tuple[float, float, float]:
        """(p_u, p_m, p_d) with p_u u + p_m m + p_d d = r for a chosen p_m.

        The risk-neutral probability is not unique in this model.
        """
        p_up = (self.r - p_mid * self.m - (1 - p_mid) * self.d) / (
            self.u - self.d
        )
        return p_up, p_mid, 1 - p_mid - p_up

    def max_middle_prob(self) -> float:
        """p_u, p_d > 0 exactly when 0 < p_m < this bound."""
        return min(
            (self.r - self.d) / (self.m - self.d),
            (self.u - self.r) / (self.u - self.m),
        )


# 3.3.2 Continuous-time limit
def norm_cdf(x: float) -> float:
    """Standard normal CDF, Abramowitz & Stegun 26.2.17 (|error| < 7.5e-8)."""
    if x < 0:
        return 1 - norm_cdf(-x)
    t = 1 / (1 + 0.2316419 * x)
    poly = t * (
        0.319381530
        + t
        * (
            -0.356563782
            + t * (1.781477937 + t * (-1.821255978 + t * 1.330274429))
        )
    )
    return 1 - math.exp(-x * x / 2) / math.sqrt(2 * math.pi) * poly


def binomial_expected_price(
    s0: float, mu: float, sigma: float, horizon: float, num_steps: int
) -> float:
    """E(S(T)) with N steps of length tau = T/N."""
    tau = horizon / num_steps
    a = mu * tau
    b = sigma * math.sqrt(tau)
    return s0 * (0.5 * math.exp(a + b) + 0.5 * math.exp(a - b)) ** num_steps


def lognormal_expected_price(
    s0: float, mu: float, sigma: float, horizon: float
) -> float:
    """Limit N -> oo: E(S(T)) = S(0) e^((mu + sigma^2/2) T)."""
    return s0 * math.exp((mu + sigma * sigma / 2) * horizon)


def binomial_prob_above(
    s0: float,
    mu: float,
    sigma: float,
    horizon: float,
    num_steps: int,
    threshold: float,
) -> float:
    """P(S(T) > K) in the N-step binomial model."""
    b = sigma * math.sqrt(horizon / num_steps)
    log_threshold = math.log(threshold / s0) - mu * horizon
    log_half_n = num_steps * math.log(0.5)
    log_c = 0.0
    total = 0.0
    for k in range(num_steps + 1):
        if (2 * k - num_steps) * b > log_threshold:
            total += math.exp(log_c + log_half_n)
        if k < num_steps:
            log_c += math.log(num_steps - k) - math.log(k + 1)
    return total


def lognormal_prob_above(
    s0: float, mu: float, sigma: float, horizon: float, threshold: float
) -> float:
    """Limit N -> oo: ln S(T) ~ N(ln S(0) + mu T, sigma^2 T)."""
    return norm_cdf(
        (math.log(s0 / threshold) + mu * horizon) / (sigma * math.sqrt(horizon))
    )
