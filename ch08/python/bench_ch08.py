"""Wall-clock microbenchmarks for chapter 8 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import math
import platform
import sys
import timeit

import binomial_pricing
import black_scholes
import ch08


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


def crr(num_steps: int) -> tuple[float, float, float]:
    """u, d and r per step of a one-year CRR tree, sigma = 0.2, r = 0.05."""
    tau = 1.0 / num_steps
    return (
        math.exp(0.2 * math.sqrt(tau)) - 1,
        math.exp(-0.2 * math.sqrt(tau)) - 1,
        math.exp(0.05 * tau) - 1,
    )


def put(s: float) -> float:
    return max(100.0 - s, 0.0)


def main() -> None:
    cases = []
    for n in (100, 2000):
        u, d, r = crr(n)
        cases.append(
            (
                f"OptionTreeAmerican/{n}",
                lambda n=n, u=u, d=d, r=r: binomial_pricing.option_tree(
                    100.0, u, d, r, n, put, american=True
                ),
            )
        )
        cases.append(
            (
                f"EarlyExerciseNodes/{n}",
                lambda n=n, u=u, d=d, r=r: (
                    binomial_pricing.early_exercise_nodes(
                        100.0, u, d, r, n, put
                    )
                ),
            )
        )
        for american in (False, True):
            kind = "American" if american else "European"
            cases.append(
                (
                    f"CrrTreePrice{kind}/{n}",
                    lambda n=n, a=american: black_scholes.crr_tree_price(
                        100.0, 100.0, 0.05, 0.2, 1.0, n, False, a
                    ),
                )
            )
    cases.append(
        (
            "BinomialSum/1000",
            lambda: binomial_pricing.binomial_sum(0.5, 1000, 500),
        )
    )
    cases.append(("Report", ch08.report))
    run(cases)


if __name__ == "__main__":
    main()
