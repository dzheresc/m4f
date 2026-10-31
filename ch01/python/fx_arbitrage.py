"""Exercise 1.3: spotting a risk-free profit among currency dealers' quotes."""

from __future__ import annotations

import dataclasses


@dataclasses.dataclass(frozen=True)
class FxQuote:
    """A dealer's quote for `base` in `quote` currency.

    The dealer buys 1 base for `buy` units of quote and sells 1 base for
    `sell` units of quote (buy < sell).
    """

    dealer: str
    base: str
    quote: str
    buy: float
    sell: float


@dataclasses.dataclass(frozen=True)
class FxLeg:
    """Converting 1 unit of `from_` into `rate` units of `to` with `dealer`."""

    from_: str
    to: str
    dealer: str
    rate: float


@dataclasses.dataclass(frozen=True)
class FxCycle:
    """A round trip; `growth` is start currency received per unit invested."""

    legs: tuple[FxLeg, ...]
    growth: float


class FxMarket:
    """All conversions offered by a set of dealer quotes."""

    def __init__(self, quotes: list[FxQuote]) -> None:
        self._legs: list[FxLeg] = []
        for q in quotes:
            # Sell base to the dealer, or buy base from the dealer.
            self._legs.append(FxLeg(q.base, q.quote, q.dealer, q.buy))
            self._legs.append(FxLeg(q.quote, q.base, q.dealer, 1.0 / q.sell))
        self._currencies = sorted(
            {c for q in quotes for c in (q.base, q.quote)}
        )

    def best_leg(self, from_: str, to: str) -> FxLeg | None:
        """Best direct conversion from_ -> to (first dealer wins ties)."""
        best = None
        for leg in self._legs:
            if (
                leg.from_ == from_
                and leg.to == to
                and (best is None or leg.rate > best.rate)
            ):
                best = leg
        return best

    def best_cycle(self, start: str) -> FxCycle:
        """Most profitable round trip start -> ... -> start.

        Searches exhaustively through distinct currencies, which is fine for a
        handful of currencies.
        """
        best = FxCycle((), 0.0)
        path: list[FxLeg] = []
        visited = [start]

        def search(at: str, growth: float) -> None:
            nonlocal best
            for nxt in self._currencies:
                leg = self.best_leg(at, nxt)
                if leg is None:
                    continue
                closes = nxt == start
                if (not closes and nxt in visited) or (closes and not path):
                    continue
                path.append(leg)
                new_growth = growth * leg.rate
                if closes:
                    if new_growth > best.growth:
                        best = FxCycle(tuple(path), new_growth)
                else:
                    visited.append(nxt)
                    search(nxt, new_growth)
                    visited.pop()
                path.pop()

        search(start, 1.0)
        return best
