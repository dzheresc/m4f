"""Sections 7.1-7.3: option payoffs, put-call parity and bounds on prices.

Continuous compounding at rate r; `maturity` is the time T to expiry in years
and `strike` the strike X. `div` is the present value of dividends paid
before T. `s0` is the stock price S(0) now and `s` the stock price S(T) at
expiry.
"""

import math


# Section 7.1: definitions.
def call_payoff(s: float, strike: float) -> float:
    """max(S - X, 0)."""
    return max(s - strike, 0.0)


def put_payoff(s: float, strike: float) -> float:
    """max(X - S, 0)."""
    return max(strike - s, 0.0)


def profit(payoff: float, premium: float, r: float, maturity: float) -> float:
    """Profit at T of a long position bought for `premium` at 0.

    The premium is financed at r; a short position earns the negative of it.
    """
    return payoff - premium * math.exp(r * maturity)


# Section 7.2: put-call parity, C - P = S(0) - div - X e^(-rT).
def parity_gap(
    call: float,
    put: float,
    s0: float,
    strike: float,
    r: float,
    maturity: float,
    div: float = 0.0,
) -> float:
    """C - P - (S(0) - div - X e^(-rT)).

    Positive: write the call, buy the put and the stock, borrow; negative: the
    reverse. Either locks in |gap| e^(rT) at T.
    """
    return call - put - (s0 - div - strike * math.exp(-r * maturity))


# Section 7.3.1: European options.
def european_call_bounds(
    s0: float, strike: float, r: float, maturity: float, div: float = 0.0
) -> tuple[float, float]:
    """max(0, S(0) - div - X e^(-rT)) <= C_E <= S(0) - div."""
    return max(0.0, s0 - div - strike * math.exp(-r * maturity)), s0 - div


def european_put_bounds(
    s0: float, strike: float, r: float, maturity: float, div: float = 0.0
) -> tuple[float, float]:
    """max(0, X e^(-rT) - S(0) + div) <= P_E <= X e^(-rT)."""
    return (
        max(0.0, strike * math.exp(-r * maturity) - s0 + div),
        strike * math.exp(-r * maturity),
    )


def bound_violation(price: float, bounds: tuple[float, float]) -> float:
    """How far a quoted price lies outside [lo, hi] (0 if inside).

    This much can be locked in at time 0 by trading against the quote.
    """
    lo, hi = bounds
    return lo - price if price < lo else (price - hi if price > hi else 0.0)


# Section 7.3.3: American options.
def american_parity_bounds(
    s0: float, strike: float, r: float, maturity: float
) -> tuple[float, float]:
    """S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) (no dividends)."""
    return s0 - strike, s0 - strike * math.exp(-r * maturity)


# Section 7.4.1: dependence on the strike, for X1 < X2 < X3.
def butterfly_gap(
    c1: float, c2: float, c3: float, x1: float, x2: float, x3: float
) -> float:
    """Convexity in X: C(X2) <= l C(X1) + (1-l) C(X3), l = (X3-X2)/(X3-X1).

    A positive return value is the arbitrage profit from writing the middle
    option and buying the wings. c1, c2, c3 are the call prices at strikes
    x1, x2, x3.
    """
    lam = (x3 - x2) / (x3 - x1)
    return c2 - (lam * c1 + (1 - lam) * c3)
