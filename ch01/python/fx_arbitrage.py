"""Exercise 1.3: spotting a risk-free profit among currency dealers' quotes.

A quote for `base` in `quote` currency: the dealer buys 1 base for `buy`
units of quote and sells 1 base for `sell` units of quote (buy < sell).
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class FxQuote:
    dealer: str
    base: str
    quote: str
    buy: float
    sell: float


@dataclass(frozen=True)
class FxLeg:
    """Converting 1 unit of `from_` into `rate` units of `to` with `dealer`."""
    from_: str
    to: str
    dealer: str
    rate: float


@dataclass(frozen=True)
class FxCycle:
    legs: tuple[FxLeg, ...]
    growth: float  # units of start currency received per unit invested


class FxMarket:
    def __init__(self, quotes: list[FxQuote]) -> None:
        self.legs: list[FxLeg] = []
        for q in quotes:
            self.legs.append(FxLeg(q.base, q.quote, q.dealer, q.buy))        # sell base to dealer
            self.legs.append(FxLeg(q.quote, q.base, q.dealer, 1.0 / q.sell))  # buy base from dealer
        self.ccys = sorted({c for q in quotes for c in (q.base, q.quote)})

    def best_leg(self, from_: str, to: str) -> FxLeg | None:
        """Best direct conversion from_ -> to (first dealer wins ties)."""
        best = None
        for l in self.legs:
            if l.from_ == from_ and l.to == to and (best is None or l.rate > best.rate):
                best = l
        return best

    def best_cycle(self, start: str) -> FxCycle:
        """Most profitable round trip start -> ... -> start through distinct
        currencies (exhaustive search; fine for a handful of currencies)."""
        best = FxCycle((), 0.0)
        path: list[FxLeg] = []
        visited = [start]

        def search(at: str, growth: float) -> None:
            nonlocal best
            for nxt in self.ccys:
                leg = self.best_leg(at, nxt)
                if leg is None:
                    continue
                closes = nxt == start
                if (not closes and nxt in visited) or (closes and not path):
                    continue
                path.append(leg)
                g = growth * leg.rate
                if closes:
                    if g > best.growth:
                        best = FxCycle(tuple(path), g)
                else:
                    visited.append(nxt)
                    search(nxt, g)
                    visited.pop()
                path.pop()

        search(start, 1.0)
        return best
