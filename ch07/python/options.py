"""Sections 7.1-7.3: option payoffs, put-call parity and bounds on option
prices. Continuous compounding at rate r; T is the time to expiry in years and
X the strike. div is the present value of dividends paid before T.
"""
from __future__ import annotations

import math


# 7.1 Definitions
def call_payoff(S: float, X: float) -> float:
    return max(S - X, 0.0)


def put_payoff(S: float, X: float) -> float:
    return max(X - S, 0.0)


def profit(payoff: float, premium: float, r: float, T: float) -> float:
    """Profit at T of a long position bought for `premium` at 0 (premium
    financed at r); a short position earns the negative of it."""
    return payoff - premium * math.exp(r * T)


# 7.2 Put-call parity: C - P = S(0) - div - X e^(-rT)
def parity_gap(C: float, P: float, S0: float, X: float, r: float, T: float, div: float = 0.0) -> float:
    """C - P - (S(0) - div - X e^(-rT)). Positive: write the call, buy the put
    and the stock, borrow; negative: the reverse. Either locks in |gap| e^(rT) at T."""
    return C - P - (S0 - div - X * math.exp(-r * T))


# 7.3.1 European options
def european_call_bounds(S0: float, X: float, r: float, T: float, div: float = 0.0) -> tuple[float, float]:
    """max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div"""
    return max(0.0, S0 - div - X * math.exp(-r * T)), S0 - div


def european_put_bounds(S0: float, X: float, r: float, T: float, div: float = 0.0) -> tuple[float, float]:
    """max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT)"""
    return max(0.0, X * math.exp(-r * T) - S0 + div), X * math.exp(-r * T)


def bound_violation(price: float, bounds: tuple[float, float]) -> float:
    """How far a quoted price lies outside [lo, hi] (0 if inside); this much
    can be locked in at time 0 by trading against the quote."""
    lo, hi = bounds
    return lo - price if price < lo else (price - hi if price > hi else 0.0)


# 7.3.3 American options
def american_parity_bounds(S0: float, X: float, r: float, T: float) -> tuple[float, float]:
    """S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends)"""
    return S0 - X, S0 - X * math.exp(-r * T)


# 7.4.1 Dependence on the strike: for X1 < X2 < X3
def butterfly_gap(C1: float, C2: float, C3: float, X1: float, X2: float, X3: float) -> float:
    """Convexity in X: C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1).
    A positive return value is the arbitrage profit from writing the middle
    option and buying the wings."""
    lam = (X3 - X2) / (X3 - X1)
    return C2 - (lam * C1 + (1 - lam) * C3)
