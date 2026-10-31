"""Sections 5.1-5.2: risk and portfolios of two securities.

Risk is the standard deviation of return; a two-security portfolio has weights
w1 + w2 = 1.
"""

from collections.abc import Sequence
import math


# Section 5.1: risk, from returns k_i in scenarios with probabilities p_i.
def mean(k: Sequence[float], p: Sequence[float]) -> float:
    """Expected return sum_i p_i k_i."""
    s = 0.0
    for k_i, p_i in zip(k, p):
        s += k_i * p_i
    return s


def covariance(
    k1: Sequence[float], k2: Sequence[float], p: Sequence[float]
) -> float:
    """Cov(K1, K2) = sum_i p_i (k1_i - mu1) (k2_i - mu2)."""
    m1, m2 = mean(k1, p), mean(k2, p)
    s = 0.0
    for a, b, p_i in zip(k1, k2, p):
        s += p_i * (a - m1) * (b - m2)
    return s


def std_dev(k: Sequence[float], p: Sequence[float]) -> float:
    """Standard deviation of the return k."""
    return math.sqrt(covariance(k, k, p))


def correlation(
    k1: Sequence[float], k2: Sequence[float], p: Sequence[float]
) -> float:
    """Correlation coefficient rho_12."""
    return covariance(k1, k2, p) / (std_dev(k1, p) * std_dev(k2, p))


# Section 5.2.1: two securities, w1 in the first and w2 = 1 - w1 in the second.
def two_asset_mean(w1: float, mu1: float, mu2: float) -> float:
    """mu_V = w1 mu1 + w2 mu2."""
    return w1 * mu1 + (1 - w1) * mu2


def two_asset_std(w1: float, s1: float, s2: float, rho: float) -> float:
    """sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2.

    Clamped at 0 against rounding when |rho| = 1 makes the portfolio riskless.
    """
    w2 = 1 - w1
    return math.sqrt(
        max(
            0.0,
            w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2 * w1 * w2 * rho * s1 * s2,
        )
    )


def min_variance_weight(s1: float, s2: float, rho: float) -> float:
    """w1 of the minimum variance portfolio.

    (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2).
    """
    return (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2)
