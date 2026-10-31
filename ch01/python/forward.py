"""Section 1.5: forward contracts.

A portfolio (x, y, z) adds z forward contracts (z > 0 long, z < 0 short) with
forward price F to x shares and y bonds; entering a forward costs nothing at
t=0. The forward price F is called `delivery_price` below; other names follow
the book's notation, lower-cased (s0 = S(0), a0 = A(0), ...).
"""

from __future__ import annotations

import dataclasses


@dataclasses.dataclass(frozen=True)
class ForwardPortfolio:
    """x shares, y bonds and z forward contracts."""

    x: float
    y: float
    z: float

    def value0(self, s0: float, a0: float) -> float:
        return self.x * s0 + self.y * a0

    def value1(self, s1: float, a1: float, delivery_price: float) -> float:
        return self.x * s1 + self.y * a1 + self.z * (s1 - delivery_price)


def long_forward_payoff(s1: float, delivery_price: float) -> float:
    return s1 - delivery_price


def short_forward_payoff(s1: float, delivery_price: float) -> float:
    return delivery_price - s1


def forward_price(s0: float, a0: float, a1: float) -> float:
    """Proposition 1.2: F = S(0) A(1) / A(0) = S(0) (1 + r)."""
    return s0 * a1 / a0


def forward_arbitrage(
    s0: float, a0: float, a1: float, delivery_price: float
) -> ForwardPortfolio | None:
    """Cash-and-carry arbitrage against a mispriced forward, one contract.

    F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair.
    F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F.
    V(0) = 0 and V(1) does not depend on S(1).

    Returns:
        The arbitrage portfolio, or None if `delivery_price` is fair.
    """
    fair = forward_price(s0, a0, a1)
    if delivery_price > fair:
        return ForwardPortfolio(1.0, -s0 / a0, -1.0)
    if delivery_price < fair:
        return ForwardPortfolio(-1.0, s0 / a0, 1.0)
    return None


def implied_foreign_rate(s0: float, delivery_price: float, r: float) -> float:
    """Exercise 1.6: the foreign rate implied by a currency forward.

    Holding the foreign currency earns the foreign rate r_f (a negative
    carrying cost), so F = S(0)(1+r)/(1+r_f) and r_f = S(0)(1+r)/F - 1.
    """
    return s0 * (1 + r) / delivery_price - 1
