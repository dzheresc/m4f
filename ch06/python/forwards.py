"""Section 6.1, forward contracts, with continuous compounding at rate r.
Times are in years; F(t, T) is the forward price at t for delivery at T.
"""
from __future__ import annotations

import math
from collections.abc import Sequence


# 6.1.1 Forward price
def forward_price(S: float, r: float, tau: float) -> float:
    """F(t, T) = S(t) e^(r (T - t)), tau = T - t."""
    return S * math.exp(r * tau)


def forward_price_dividends(S0: float, r: float, T: float,
                            dividends: Sequence[tuple[float, float]]) -> float:
    """Dividends (amount, time) paid before T: F(0, T) = (S(0) - PV(div)) e^(r T)."""
    pv = 0.0
    for amount, t in dividends:
        pv += amount * math.exp(-r * t)
    return (S0 - pv) * math.exp(r * T)


def forward_price_yield(S0: float, r: float, r_div: float, T: float) -> float:
    """Continuous dividend yield r_div (or a foreign interest rate for
    currencies): F(0, T) = S(0) e^((r - r_div) T)."""
    return S0 * math.exp((r - r_div) * T)


def forward_arbitrage_profit(F_quoted: float, S0: float, r: float, T: float) -> float:
    """Riskless profit at T per unit from a mispriced forward. If F_quoted is
    above S(0) e^(rT): borrow S(0), buy the stock, sell forward (cash and carry);
    if below: short the stock, invest S(0), buy forward. Either way the profit is
    |F_quoted - S(0) e^(rT)|."""
    return abs(F_quoted - forward_price(S0, r, T))


# 6.1.2 Value of a forward contract
def long_forward_value(F0: float, Ft: float, r: float, tau: float) -> float:
    """Value at t of a long forward entered at 0: V(t) = (F(t,T) - F(0,T)) e^(-r (T-t))."""
    return (Ft - F0) * math.exp(-r * tau)
