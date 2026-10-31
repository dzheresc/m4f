"""The one-step binomial market of Chapter 1.

Mathematics for Finance (Capinski & Zastawniak), Chapter 1:
"Introduction: A Simple Market Model".

Two assets, two time instants t = 0, 1:
  risk-free bond  A(0) -> A(1)                     (return r)
  risky stock     S(0) -> S^u(1) with prob. p      (return u)
                        S^d(1) with prob. 1 - p    (return d)
A portfolio (x, y) holds x shares and y bonds; negative = short / borrowed.

Names follow the book's notation (Google Python style guide, section 3.16.5),
lower-cased: a0 = A(0), a1 = A(1), s0 = S(0), s_up = S^u(1), s_down = S^d(1),
and p, r, u, d, x, y as in the book.
"""

from __future__ import annotations

import dataclasses
import enum
import math


class State(enum.Enum):
    UP = "up"
    DOWN = "down"


@dataclasses.dataclass(frozen=True)
class Portfolio:
    """A holding of x shares of stock and y units of bond."""

    x: float
    y: float


@dataclasses.dataclass(frozen=True)
class OneStepMarket:
    """Bond A(0) -> A(1) and stock S(0) -> S^u(1) | S^d(1) with prob. p.

    Raises:
        ValueError: if a price is not positive, s_up <= s_down, or p is not in
            (0, 1).
    """

    a0: float
    a1: float
    s0: float
    s_up: float
    s_down: float
    p: float

    def __post_init__(self) -> None:
        if not (
            self.a0 > 0
            and self.a1 > 0
            and self.s0 > 0
            and 0 < self.s_down < self.s_up
        ):
            raise ValueError("require positive prices and S^u(1) > S^d(1)")
        if not 0 < self.p < 1:
            raise ValueError("require 0 < p < 1")

    def s1(self, s: State) -> float:
        return self.s_up if s is State.UP else self.s_down

    @property
    def r(self) -> float:
        """Bond return K_A = (A(1) - A(0)) / A(0)."""
        return (self.a1 - self.a0) / self.a0

    @property
    def u(self) -> float:
        """Stock return in the up state."""
        return (self.s_up - self.s0) / self.s0

    @property
    def d(self) -> float:
        """Stock return in the down state."""
        return (self.s_down - self.s0) / self.s0

    @property
    def is_arbitrage_free(self) -> bool:
        """Theorem: no arbitrage <=> d < r < u."""
        return self.d < self.r < self.u

    @property
    def risk_neutral_prob(self) -> float:
        """p* = (r - d) / (u - d); makes E*[S(1)] = (1 + r) S(0)."""
        return (self.r - self.d) / (self.u - self.d)

    def split(self, budget: float, w: float) -> Portfolio:
        """Invests `budget` with fraction `w` in stock, the rest in bonds."""
        return Portfolio(w * budget / self.s0, (1 - w) * budget / self.a0)

    def value0(self, portfolio: Portfolio) -> float:
        return portfolio.x * self.s0 + portfolio.y * self.a0

    def value1(self, portfolio: Portfolio, s: State) -> float:
        return portfolio.x * self.s1(s) + portfolio.y * self.a1

    def portfolio_return(self, portfolio: Portfolio, s: State) -> float:
        v0 = self.value0(portfolio)
        return (self.value1(portfolio, s) - v0) / v0

    def expect(self, up: float, down: float) -> float:
        """Expectation of a variable equal to `up` (prob. p) or `down`."""
        return self.p * up + (1 - self.p) * down

    def stddev(self, up: float, down: float) -> float:
        """Standard deviation of a variable equal to `up` or `down`."""
        return math.sqrt(self.p * (1 - self.p)) * abs(up - down)

    def replicate(self, d_up: float, d_down: float) -> Portfolio:
        """Replicates a derivative paying `d_up` / `d_down` at t=1.

        Solves x S^u(1) + y A(1) = D^u and x S^d(1) + y A(1) = D^d.

        Args:
            d_up: Payoff D^u in the up state.
            d_down: Payoff D^d in the down state.

        Returns:
            The replicating portfolio (x, y).
        """
        x = (d_up - d_down) / (self.s_up - self.s_down)
        return Portfolio(x, (d_up - x * self.s_up) / self.a1)

    def price(self, d_up: float, d_down: float) -> float:
        """No-arbitrage price D(0) = value of the replicating portfolio."""
        return self.value0(self.replicate(d_up, d_down))

    def risk_neutral_price(self, d_up: float, d_down: float) -> float:
        """D(0) as the discounted risk-neutral expectation; equals price()."""
        q = self.risk_neutral_prob
        return (q * d_up + (1 - q) * d_down) / (1 + self.r)

    @property
    def forward_price(self) -> float:
        """Forward price for delivery at t=1: F = S(0) (1 + r)."""
        return self.s0 * (1 + self.r)

    def arbitrage(self) -> Portfolio | None:
        """Builds an arbitrage strategy if the model admits one.

        Returns:
            A portfolio with V(0) = 0, V(1) >= 0 and V(1) > 0 in some state,
            using one share; None if the market is arbitrage-free.
        """
        if self.r >= self.u:
            # Short one share and buy bonds with the proceeds.
            return Portfolio(-1.0, self.s0 / self.a0)
        if self.r <= self.d:
            # Borrow to buy one share.
            return Portfolio(1.0, -self.s0 / self.a0)
        return None


def call_payoff(s: float, strike: float) -> float:
    return s - strike if s > strike else 0.0


def put_payoff(s: float, strike: float) -> float:
    return strike - s if strike > s else 0.0


def hedged_purchase_cost(
    market: OneStepMarket, strike: float, num_calls: float, s: State
) -> float:
    """Section 1.7: cost at t=1 of buying one share, hedged with calls.

    The investor borrowed at t=0 to buy `num_calls` calls with strike
    `strike`, so the cost is S(1) - n C(1) + n C(0) (1 + r).
    """
    call0 = market.price(
        call_payoff(market.s_up, strike), call_payoff(market.s_down, strike)
    )
    return (
        market.s1(s)
        - num_calls * call_payoff(market.s1(s), strike)
        + num_calls * call0 * (1 + market.r)
    )
