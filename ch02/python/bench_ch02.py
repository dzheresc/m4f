"""Wall-clock microbenchmarks for chapter 2 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import platform
import sys
import timeit

import ch02
import money_market


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


def main() -> None:
    cases = []
    for n in (3, 30, 360):
        price = money_market.coupon_bond_price(100.0, 8.0, n, 0.07)
        cases.append(
            (
                f"CouponBondPrice/{n}",
                lambda n=n: money_market.coupon_bond_price(100.0, 8.0, n, 0.07),
            )
        )
        cases.append(
            (
                f"YieldToMaturity/{n}",
                lambda n=n, p=price: money_market.yield_to_maturity(
                    p, 100.0, 8.0, n
                ),
            )
        )
    cases.append(("Report", ch02.report))
    run(cases)


if __name__ == "__main__":
    main()
