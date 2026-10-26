"""Section 3.1, dynamics of stock prices: returns, logarithmic returns and
their expectations over a finite set of scenarios.
"""
from __future__ import annotations

import math
from collections.abc import Sequence


# 3.1.1 Return
def one_step_return(S0: float, S1: float, dividend: float = 0.0) -> float:
    """K(1) = (S(1) + div - S(0)) / S(0)."""
    return (S1 + dividend - S0) / S0


def log_return(S0: float, S1: float) -> float:
    """k(1) = ln(S(1) / S(0))."""
    return math.log(S1 / S0)


def returns(path: Sequence[float]) -> list[float]:
    """One-step returns K(n) along a price path S(0), S(1), ..."""
    return [one_step_return(path[i - 1], path[i]) for i in range(1, len(path))]


def log_returns(path: Sequence[float]) -> list[float]:
    return [log_return(path[i - 1], path[i]) for i in range(1, len(path))]


def compound_return(Ks: Sequence[float]) -> float:
    """K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1)) ... (1 + K(n))."""
    g = 1.0
    for K in Ks:
        g *= 1 + K
    return g - 1


# 3.1.2 Expected return
def expectation(values: Sequence[float], probs: Sequence[float]) -> float:
    s = 0.0
    for v, p in zip(values, probs):
        s += v * p
    return s


def variance(values: Sequence[float], probs: Sequence[float]) -> float:
    mu = expectation(values, probs)
    s = 0.0
    for v, p in zip(values, probs):
        d = v - mu
        s += p * d * d
    return s
