"""Wall-clock microbenchmarks for chapter 5 (Python).

Run from the chapter directory as ./bench.sh --python [filter].
"""

from __future__ import annotations

from collections.abc import Callable
import platform
import random
import sys
import timeit

import ch05
import portfolio
import risk


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


def make_cov(n: int) -> list[list[float]]:
    """A positive definite covariance matrix from a two-factor model."""
    rng = random.Random(n)
    factors = [
        (rng.uniform(-0.2, 0.3), rng.uniform(-0.2, 0.3)) for _ in range(n)
    ]
    cov = [[fi[0] * fj[0] + fi[1] * fj[1] for fj in factors] for fi in factors]
    for i in range(n):
        cov[i][i] += rng.uniform(0.01, 0.04)
    return cov


def main() -> None:
    cases = []
    for n in (3, 50, 200):
        cov = make_cov(n)
        w = [1.0 / n] * n
        cases.append(
            (
                f"MinVariancePortfolio/{n}",
                lambda c=cov: portfolio.min_variance_portfolio(c),
            )
        )
        cases.append(
            (f"VecMat/{n}", lambda w=w, c=cov: portfolio.vec_mat(w, c))
        )
    for n in (3, 10000):
        rng = random.Random(n)
        k1 = [rng.uniform(-0.1, 0.2) for _ in range(n)]
        k2 = [rng.uniform(-0.1, 0.2) for _ in range(n)]
        p = [1.0 / n] * n
        cases.append(
            (
                f"Correlation/{n}",
                lambda k1=k1, k2=k2, p=p: risk.correlation(k1, k2, p),
            )
        )
    cases.append(("Report", ch05.report))
    run(cases)


if __name__ == "__main__":
    main()
