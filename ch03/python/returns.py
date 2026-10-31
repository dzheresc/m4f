"""Section 3.1, dynamics of stock prices.

Returns, logarithmic returns and their expectations over a finite set of
scenarios. Book notation: S(0), S(1) are `s0`, `s1`; K is a one-step return
and k a logarithmic return.
"""

from __future__ import annotations

from collections.abc import Sequence
import math


# 3.1.1 Return
def one_step_return(s0: float, s1: float, dividend: float = 0.0) -> float:
    """K(1) = (S(1) + div - S(0)) / S(0)."""
    return (s1 + dividend - s0) / s0


def log_return(s0: float, s1: float) -> float:
    """k(1) = ln(S(1) / S(0))."""
    return math.log(s1 / s0)


def returns(path: Sequence[float]) -> list[float]:
    """One-step returns K(n) along a price path S(0), S(1), ..."""
    return [one_step_return(path[i - 1], path[i]) for i in range(1, len(path))]


def log_returns(path: Sequence[float]) -> list[float]:
    """One-step logarithmic returns k(n) along a price path."""
    return [log_return(path[i - 1], path[i]) for i in range(1, len(path))]


def compound_return(one_step_returns: Sequence[float]) -> float:
    """K(0, n) from one-step returns: 1 + K(0, n) = (1 + K(1))...(1 + K(n))."""
    growth = 1.0
    for k in one_step_returns:
        growth *= 1 + k
    return growth - 1


# 3.1.2 Expected return
def expectation(values: Sequence[float], probs: Sequence[float]) -> float:
    total = 0.0
    for value, prob in zip(values, probs):
        total += value * prob
    return total


def variance(values: Sequence[float], probs: Sequence[float]) -> float:
    mean = expectation(values, probs)
    total = 0.0
    for value, prob in zip(values, probs):
        deviation = value - mean
        total += prob * deviation * deviation
    return total
