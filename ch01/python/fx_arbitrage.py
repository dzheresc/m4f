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
        self._index = {c: i for i, c in enumerate(self._currencies)}
        # best[i][j]: index into _legs of the best leg from currency i to
        # currency j, or None. Built once so that the search does no string
        # comparisons. Legs are scanned in order and only a strictly better
        # rate replaces the current best, so the first dealer wins ties.
        n = len(self._currencies)
        self._best: list[list[int | None]] = [[None] * n for _ in range(n)]
        for k, leg in enumerate(self._legs):
            row = self._best[self._index[leg.from_]]
            j = self._index[leg.to]
            best = row[j]
            if best is None or leg.rate > self._legs[best].rate:
                row[j] = k

    def best_leg(self, from_: str, to: str) -> FxLeg | None:
        """Best direct conversion from_ -> to (first dealer wins ties)."""
        i = self._index.get(from_)
        j = self._index.get(to)
        if i is None or j is None:
            return None
        k = self._best[i][j]
        return None if k is None else self._legs[k]

    def best_cycle(self, start: str) -> FxCycle:
        """Most profitable round trip start -> ... -> start.

        Searches exhaustively through distinct currencies, by currency and leg
        index, which is fine for a handful of currencies.
        """
        s = self._index.get(start)
        if s is None:
            return FxCycle((), 0.0)
        legs = self._legs
        best_table = self._best
        visited = [False] * len(self._currencies)
        visited[s] = True
        path: list[int] = []
        best_path: list[int] = []
        best_growth = 0.0

        def search(at: int, growth: float) -> None:
            nonlocal best_path, best_growth
            for nxt, k in enumerate(best_table[at]):
                if k is None:
                    continue
                closes = nxt == s
                if (not closes and visited[nxt]) or (closes and not path):
                    continue
                path.append(k)
                new_growth = growth * legs[k].rate
                if closes:
                    if new_growth > best_growth:
                        best_growth = new_growth
                        best_path = path.copy()
                else:
                    visited[nxt] = True
                    search(nxt, new_growth)
                    visited[nxt] = False
                path.pop()

        search(s, 1.0)
        return FxCycle(tuple(legs[k] for k in best_path), best_growth)
