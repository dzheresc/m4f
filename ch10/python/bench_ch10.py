"""Wall-clock microbenchmarks for chapter 10 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import math
import platform
import random
import sys
import timeit

import bonds
import ch10
import term_structure


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


def make_curve(n: int) -> tuple[list[float], list[float]]:
    """A smooth upward-sloping zero curve at n annual maturities."""
    maturities = [float(i) for i in range(1, n + 1)]
    prices = [
        math.exp(-(0.03 + 0.02 * (1 - math.exp(-t / 5))) * t)
        for t in maturities
    ]
    return maturities, prices


def main() -> None:
    cases = []
    for n in (3, 30, 360):
        maturities, prices = make_curve(n)
        amounts = [5.0] * n
        amounts[-1] += 100.0
        price = term_structure.coupon_bond_price(
            maturities, amounts, maturities, prices
        )
        cases.append(
            (
                f"CouponBondPrice/{n}",
                lambda m=maturities, a=amounts, p=prices: (
                    term_structure.coupon_bond_price(m, a, m, p)
                ),
            )
        )
        cases.append(
            (
                f"YieldToMaturity/{n}",
                lambda b=price, m=maturities, a=amounts: (
                    term_structure.yield_to_maturity(b, m, a)
                ),
            )
        )
    # The report's bonds, both alive over the 5-year horizon.
    b1 = bonds.coupon_bond(100.0, 4.0, 5)
    b2 = bonds.coupon_bond(100.0, 8.0, 10)
    for steps in (5, 260):
        rng = random.Random(steps)
        path = [(0.0, 0.06)]
        for i in range(1, steps + 1):
            shock = rng.gauss(0.0, 0.01 / math.sqrt(steps / 5.0))
            path.append((5.0 * i / steps, path[-1][1] + shock))
        cases.append(
            (
                f"DynamicHedge/{steps}",
                lambda p=path: bonds.dynamic_hedge(1000.0, b1, b2, 5.0, p),
            )
        )
    cases.append(("Report", ch10.report))
    run(cases)


if __name__ == "__main__":
    main()
