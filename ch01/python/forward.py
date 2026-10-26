"""Section 1.5: forward contracts.

A portfolio (x, y, z) adds z forward contracts (z > 0 long, z < 0 short) with
forward price F to x shares and y bonds; entering a forward costs nothing at t=0.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ForwardPortfolio:
    x: float  # shares
    y: float  # bonds
    z: float  # forward contracts

    def value0(self, S0: float, A0: float) -> float:
        return self.x * S0 + self.y * A0

    def value1(self, S1: float, A1: float, F: float) -> float:
        return self.x * S1 + self.y * A1 + self.z * (S1 - F)


def long_forward_payoff(S1: float, F: float) -> float:
    return S1 - F


def short_forward_payoff(S1: float, F: float) -> float:
    return F - S1


def forward_price(S0: float, A0: float, A1: float) -> float:
    """Proposition 1.2: the No-Arbitrage forward price F = S(0) A(1) / A(0) = S(0)(1+r)."""
    return S0 * A1 / A0


def forward_arbitrage(S0: float, A0: float, A1: float, F: float) -> ForwardPortfolio | None:
    """Cash-and-carry arbitrage against a mispriced forward, one contract:
      F > fair: borrow S(0), buy a share, short the forward  -> V(1) = F - fair
      F < fair: short a share, invest S(0), long the forward -> V(1) = fair - F
    V(0) = 0 and V(1) does not depend on S(1).
    """
    fair = forward_price(S0, A0, A1)
    if F > fair:
        return ForwardPortfolio(1.0, -S0 / A0, -1.0)
    if F < fair:
        return ForwardPortfolio(-1.0, S0 / A0, 1.0)
    return None


def implied_foreign_rate(S0: float, F: float, r: float) -> float:
    """Exercise 1.6: forward on a currency. Holding the foreign currency earns the
    foreign rate r_f (a negative carrying cost), so F = S(0)(1+r)/(1+r_f) and
    r_f = S(0)(1+r)/F - 1."""
    return S0 * (1 + r) / F - 1
