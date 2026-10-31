"""Wall-clock microbenchmarks for chapter 1 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import platform
import random
import statistics
import sys
import timeit

import ch01
import fx_arbitrage


def _format(seconds: float) -> str:
    for unit, scale in (("s", 1.0), ("ms", 1e-3), ("us", 1e-6)):
        if seconds >= scale:
            return f"{seconds / scale:.4g} {unit}"
    return f"{seconds / 1e-9:.4g} ns"


def run(cases: list[tuple[str, Callable[[], object]]]) -> None:
    """Times each case whose name contains sys.argv[1] (all by default).

    Prints the median time per call over 5 repeats (3 for slow cases), each
    repeat long enough for timeit.autorange (at least 0.2 s).
    """
    pattern = sys.argv[1] if len(sys.argv) > 1 else ""
    print(f"Python {platform.python_version()}")
    for name, func in cases:
        if pattern not in name:
            continue
        timer = timeit.Timer(func)
        number, total = timer.autorange()
        times = timer.repeat(repeat=3 if total > 2 else 5, number=number)
        per_call = statistics.median(times) / number
        print(f"{name:<34} {_format(per_call):>12}", flush=True)


def book_quotes() -> list[fx_arbitrage.FxQuote]:
    """Exercise 1.3: dealers A (New York) and B (London)."""
    return [
        fx_arbitrage.FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
        fx_arbitrage.FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
        fx_arbitrage.FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
        fx_arbitrage.FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
    ]


def synthetic_quotes(n: int) -> list[fx_arbitrage.FxQuote]:
    """One dealer quoting every pair of n currencies, 0.5% spread, noise."""
    rng = random.Random(n)
    mid = [rng.uniform(0.5, 2.0) for _ in range(n)]
    quotes = []
    for i in range(n):
        for j in range(i + 1, n):
            rate = mid[i] / mid[j] * (1 + rng.uniform(-0.004, 0.004))
            quotes.append(
                fx_arbitrage.FxQuote(
                    "D", f"C{i}", f"C{j}", rate * 0.9975, rate * 1.0025
                )
            )
    return quotes


def main() -> None:
    book = fx_arbitrage.FxMarket(book_quotes())
    cases = [
        ("FxMarketConstruct", lambda: fx_arbitrage.FxMarket(book_quotes())),
        ("BestLeg", lambda: book.best_leg("USD", "GBP")),
        ("BestCycleBook", lambda: book.best_cycle("USD")),
    ]
    for n in (3, 6, 8):
        market = fx_arbitrage.FxMarket(synthetic_quotes(n))
        cases.append(
            (f"BestCycleScaling/{n}", lambda m=market: m.best_cycle("C0"))
        )
    cases.append(("Report", ch01.report))
    run(cases)


if __name__ == "__main__":
    main()
