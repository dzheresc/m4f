"""Wall-clock microbenchmarks for chapter 4 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable, Sequence
import platform
import random
import sys
import timeit

import ch04
import no_arbitrage
import strategies


def _format(seconds: float) -> str:
    for unit, scale in (("s", 1.0), ("ms", 1e-3), ("us", 1e-6)):
        if seconds >= scale:
            return f"{seconds / scale:.4g} {unit}"
    return f"{seconds / 1e-9:.4g} ns"


def run(cases: list[tuple[str, Callable[[], object]]]) -> None:
    """Times each case whose name contains sys.argv[1] (all by default).

    Prints the best time per call over 5 repeats (3 for slow cases), each
    repeat long enough for timeit.autorange (at least 0.2 s). The minimum is
    the least disturbed by other load, as the timeit documentation advises.
    """
    pattern = sys.argv[1] if len(sys.argv) > 1 else ""
    print(f"Python {platform.python_version()}")
    for name, func in cases:
        if pattern not in name:
            continue
        timer = timeit.Timer(func)
        number, total = timer.autorange()
        times = timer.repeat(repeat=3 if total > 2 else 5, number=number)
        per_call = min(times) / number
        print(f"{name:<34} {_format(per_call):>12}", flush=True)


def contrarian(_: int, moves: Sequence[int]) -> float:
    """Holds one more share for every down move so far."""
    return 1.0 + sum(1.0 for move in moves if move == 0)


def main() -> None:
    q = (0.03 - -0.05) / (0.10 - -0.05)
    cases = []
    for n in (4, 12, 16):
        cases.append(
            (
                f"ExpectedDiscountedValue/{n}",
                lambda n=n: no_arbitrage.expected_discounted_value(
                    100.0, 0.10, -0.05, 0.03, n, 100.0, contrarian, q
                ),
            )
        )
    for n in (12, 1000):
        rng = random.Random(n)
        s = [100.0 * (1 + 0.01 * rng.uniform(-1, 1)) for _ in range(n + 1)]
        a = [1.001**i for i in range(n + 1)]
        x = [rng.uniform(0, 2) for _ in range(n)]
        cases.append(
            (
                f"SelfFinancing/{n}",
                lambda s=s, a=a, x=x: strategies.self_financing(100.0, s, a, x),
            )
        )
    cases.append(("Report", ch04.report))
    run(cases)


if __name__ == "__main__":
    main()
