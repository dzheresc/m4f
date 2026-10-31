"""Wall-clock microbenchmarks for chapter 11 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import platform
import statistics
import sys
import timeit

import ch11
import derivatives
import rate_tree


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


TAU = 1.0 / 12


def main() -> None:
    cases = []
    for n in (10, 50, 200):
        rates = rate_tree.rate_lattice(0.05, 0.002, -0.002, n)
        probs = rate_tree.constant_probs(0.5, n)
        cases.append(
            (
                f"BondPrices/{n}",
                lambda r=rates, p=probs, n=n: rate_tree.bond_prices(
                    r, p, TAU, n
                ),
            )
        )
    for n in (100, 1000):
        rates = rate_tree.rate_lattice(0.05, 0.002, -0.002, n)
        probs = rate_tree.constant_probs(0.5, n)
        cases.append(
            (
                f"Caplet/{n}",
                lambda r=rates, p=probs, n=n: derivatives.caplet(
                    r, p, TAU, n - 1, 0.004
                ),
            )
        )
        cases.append(
            (
                f"RateLattice/{n}",
                lambda n=n: rate_tree.rate_lattice(0.05, 0.002, -0.002, n),
            )
        )
    cases.append(("Report", ch11.report))
    run(cases)


if __name__ == "__main__":
    main()
