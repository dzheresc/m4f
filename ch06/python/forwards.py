"""Section 6.1, forward contracts, with continuous compounding at rate r.

Times are in years; F(t, T) is the forward price at t for delivery at T.
Names: s = S(t) and s0 = S(0) are spot prices, f0 = F(0, T) and
ft = F(t, T) forward prices, and tau = T - t the time left to delivery.
"""

from collections.abc import Sequence
import math


# Section 6.1.1: forward price.
def forward_price(s: float, r: float, tau: float) -> float:
    """F(t, T) = S(t) e^(r (T - t)), tau = T - t."""
    return s * math.exp(r * tau)


def forward_price_dividends(
    s0: float,
    r: float,
    maturity: float,
    dividends: Sequence[tuple[float, float]],
) -> float:
    """Forward price with dividends (amount, time) paid before T.

    F(0, T) = (S(0) - PV(div)) e^(r T).
    """
    pv = 0.0
    for amount, t in dividends:
        pv += amount * math.exp(-r * t)
    return (s0 - pv) * math.exp(r * maturity)


def forward_price_yield(
    s0: float, r: float, r_div: float, maturity: float
) -> float:
    """Forward price with a continuous dividend yield r_div.

    r_div may also be a foreign interest rate for currencies:
    F(0, T) = S(0) e^((r - r_div) T).
    """
    return s0 * math.exp((r - r_div) * maturity)


def forward_arbitrage_profit(
    quoted: float, s0: float, r: float, maturity: float
) -> float:
    """Riskless profit at T per unit from a mispriced forward.

    If `quoted` is above S(0) e^(rT): borrow S(0), buy the stock, sell forward
    (cash and carry); if below: short the stock, invest S(0), buy forward.
    Either way the profit is |quoted - S(0) e^(rT)|.
    """
    return abs(quoted - forward_price(s0, r, maturity))


# Section 6.1.2: value of a forward contract.
def long_forward_value(f0: float, ft: float, r: float, tau: float) -> float:
    """Value at t of a long forward entered at 0.

    V(t) = (F(t,T) - F(0,T)) e^(-r (T-t)).
    """
    return (ft - f0) * math.exp(-r * tau)
