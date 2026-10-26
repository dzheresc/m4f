"""Sections 5.1-5.2: risk as the standard deviation of return, and portfolios
of two securities with weights w1 + w2 = 1.
"""
from __future__ import annotations

import math
from collections.abc import Sequence


# 5.1 Risk, from returns K_i in scenarios with probabilities p_i
def mean(K: Sequence[float], p: Sequence[float]) -> float:
    s = 0.0
    for k, q in zip(K, p):
        s += k * q
    return s


def covariance(K1: Sequence[float], K2: Sequence[float], p: Sequence[float]) -> float:
    m1, m2 = mean(K1, p), mean(K2, p)
    s = 0.0
    for a, b, q in zip(K1, K2, p):
        s += q * (a - m1) * (b - m2)
    return s


def std_dev(K: Sequence[float], p: Sequence[float]) -> float:
    return math.sqrt(covariance(K, K, p))


def correlation(K1: Sequence[float], K2: Sequence[float], p: Sequence[float]) -> float:
    return covariance(K1, K2, p) / (std_dev(K1, p) * std_dev(K2, p))


# 5.2.1 Two securities: w1 in the first, w2 = 1 - w1 in the second
def two_asset_mean(w1: float, mu1: float, mu2: float) -> float:
    """mu_V = w1 mu1 + w2 mu2"""
    return w1 * mu1 + (1 - w1) * mu2


def two_asset_std(w1: float, s1: float, s2: float, rho: float) -> float:
    """sigma_V^2 = w1^2 s1^2 + w2^2 s2^2 + 2 w1 w2 rho s1 s2; clamped at 0
    against rounding when |rho| = 1 makes the portfolio riskless."""
    w2 = 1 - w1
    return math.sqrt(max(0.0, w1 * w1 * s1 * s1 + w2 * w2 * s2 * s2 + 2 * w1 * w2 * rho * s1 * s2))


def min_variance_weight(s1: float, s2: float, rho: float) -> float:
    """w1 of the minimum variance portfolio:
    (s2^2 - rho s1 s2) / (s1^2 + s2^2 - 2 rho s1 s2)"""
    return (s2 * s2 - rho * s1 * s2) / (s1 * s1 + s2 * s2 - 2 * rho * s1 * s2)
