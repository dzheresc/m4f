"""Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
"Introduction: A Simple Market Model" -- the one-step binomial market.

Two assets, two time instants t = 0, 1:
  risk-free bond  A(0) -> A(1)                     (return r)
  risky stock     S(0) -> S^u(1) with prob. p      (return u)
                        S^d(1) with prob. 1 - p    (return d)
A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from enum import Enum


class State(Enum):
    UP = "up"
    DOWN = "down"


@dataclass(frozen=True)
class Portfolio:
    x: float  # shares of stock
    y: float  # units of bond


@dataclass(frozen=True)
class OneStepMarket:
    A0: float
    A1: float
    S0: float
    Su: float
    Sd: float
    p: float

    def __post_init__(self) -> None:
        if not (self.A0 > 0 and self.A1 > 0 and self.S0 > 0 and 0 < self.Sd < self.Su):
            raise ValueError("require positive prices and S^u(1) > S^d(1)")
        if not 0 < self.p < 1:
            raise ValueError("require 0 < p < 1")

    def S1(self, s: State) -> float:
        return self.Su if s is State.UP else self.Sd

    # Returns K = (P(1) - P(0)) / P(0)
    @property
    def r(self) -> float:
        return (self.A1 - self.A0) / self.A0

    @property
    def u(self) -> float:
        return (self.Su - self.S0) / self.S0

    @property
    def d(self) -> float:
        return (self.Sd - self.S0) / self.S0

    # Theorem: no arbitrage  <=>  d < r < u
    @property
    def is_arbitrage_free(self) -> bool:
        return self.d < self.r < self.u

    # Risk-neutral probability p* = (r - d) / (u - d); makes E*[S(1)] = (1+r) S(0)
    @property
    def risk_neutral_prob(self) -> float:
        return (self.r - self.d) / (self.u - self.d)

    def split(self, budget: float, w: float) -> Portfolio:
        """Portfolio investing `budget` with fraction `w` in stock, the rest in bonds."""
        return Portfolio(w * budget / self.S0, (1 - w) * budget / self.A0)

    def value0(self, P: Portfolio) -> float:
        return P.x * self.S0 + P.y * self.A0

    def value1(self, P: Portfolio, s: State) -> float:
        return P.x * self.S1(s) + P.y * self.A1

    def portfolio_return(self, P: Portfolio, s: State) -> float:
        return (self.value1(P, s) - self.value0(P)) / self.value0(P)

    # Expectation and standard deviation of a two-point random variable
    # taking values `up` (prob p) and `down` (prob 1-p).
    def expect(self, up: float, down: float) -> float:
        return self.p * up + (1 - self.p) * down

    def stddev(self, up: float, down: float) -> float:
        return math.sqrt(self.p * (1 - self.p)) * abs(up - down)

    # Replicating portfolio for a derivative paying Du (up) / Dd (down) at t=1:
    #   x S^u(1) + y A(1) = Du,   x S^d(1) + y A(1) = Dd
    def replicate(self, Du: float, Dd: float) -> Portfolio:
        x = (Du - Dd) / (self.Su - self.Sd)
        return Portfolio(x, (Du - x * self.Su) / self.A1)

    # No-arbitrage price D(0) = value of the replicating portfolio ...
    def price(self, Du: float, Dd: float) -> float:
        return self.value0(self.replicate(Du, Dd))

    # ... which equals the discounted risk-neutral expectation.
    def risk_neutral_price(self, Du: float, Dd: float) -> float:
        q = self.risk_neutral_prob
        return (q * Du + (1 - q) * Dd) / (1 + self.r)

    # Forward price for delivery at t=1: F = S(0) (1 + r)
    @property
    def forward_price(self) -> float:
        return self.S0 * (1 + self.r)

    def arbitrage(self) -> Portfolio | None:
        """If the model admits arbitrage, a portfolio with V(0)=0, V(1)>=0 and
        V(1)>0 in some state, using one share; otherwise None."""
        if self.r >= self.u:
            return Portfolio(-1.0, self.S0 / self.A0)  # short stock, buy bonds
        if self.r <= self.d:
            return Portfolio(1.0, -self.S0 / self.A0)  # borrow, buy stock
        return None


def call_payoff(S: float, X: float) -> float:
    return S - X if S > X else 0.0


def put_payoff(S: float, X: float) -> float:
    return X - S if X > S else 0.0


def hedged_purchase_cost(m: OneStepMarket, X: float, n: float, s: State) -> float:
    """Section 1.7: cost at t=1 of buying one share for an investor who at t=0
    borrowed to buy n calls with strike X: S(1) - n C(1) + n C(0)(1+r)."""
    C0 = m.price(call_payoff(m.Su, X), call_payoff(m.Sd, X))
    return m.S1(s) - n * call_payoff(m.S1(s), X) + n * C0 * (1 + m.r)
