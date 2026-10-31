"""Section 4.1, stock and money market models: investment strategies.

Prices are given along one path: s[n] (or s[j][n] for several stocks) and
a[n] for n = 0..N, standing for the book's S(n) and A(n). A strategy holds
x(n) shares and y(n) units of the money market over the step from n-1 to n,
chosen at time n-1; lists are indexed from 0, so x[0] is x(1).
"""

from __future__ import annotations

from collections.abc import Sequence


def self_financing_multi(
    v0: float,
    s: Sequence[Sequence[float]],
    a: Sequence[float],
    x: Sequence[Sequence[float]],
    dividends: Sequence[Sequence[float]] | None = None,
) -> tuple[list[float], list[float]]:
    """Self-financing strategy in several stocks.

    Dividends dividends[j][n] are paid at n to the holders over (n-1, n]. The
    money market position y(n+1) is set so that rebalancing at n costs exactly
    V(n):
        sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
        V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).

    Args:
        v0: Initial value V(0).
        s: Stock prices s[j][n].
        a: Money market prices a[n].
        x: Stock positions x[j][n-1] = x_j(n).
        dividends: Optional dividends dividends[j][n].

    Returns:
        (y, values) with y[n-1] = y(n) and values[n] = V(n).
    """
    num_steps = len(a) - 1
    values = [v0]
    y: list[float] = []
    for n in range(1, num_steps + 1):
        stock = 0.0
        for j in range(len(s)):
            stock += x[j][n - 1] * s[j][n - 1]
        y.append((values[n - 1] - stock) / a[n - 1])
        value = y[n - 1] * a[n]
        for j in range(len(s)):
            dividend = dividends[j][n] if dividends is not None else 0.0
            value += x[j][n - 1] * (s[j][n] + dividend)
        values.append(value)
    return y, values


def self_financing(
    v0: float, s: Sequence[float], a: Sequence[float], x: Sequence[float]
) -> tuple[list[float], list[float]]:
    """One stock: y(n) and V(n) of the self-financing strategy.

    Same arithmetic as self_financing_multi with one stock and no dividends,
    without the inner loops.
    """
    values = [v0]
    y: list[float] = []
    for n in range(1, len(a)):
        y_n = (values[n - 1] - x[n - 1] * s[n - 1]) / a[n - 1]
        y.append(y_n)
        values.append(y_n * a[n] + x[n - 1] * s[n])
    return y, values


def gains(
    s: Sequence[float],
    a: Sequence[float],
    x: Sequence[float],
    y: Sequence[float],
) -> float:
    """V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1))."""
    total = 0.0
    for n in range(1, len(a)):
        total += x[n - 1] * (s[n] - s[n - 1]) + y[n - 1] * (a[n] - a[n - 1])
    return total


def discounted_gains(
    s: Sequence[float], a: Sequence[float], x: Sequence[float]
) -> float:
    """V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), S~(n) = S(n) / A(n)."""
    total = 0.0
    for n in range(1, len(a)):
        total += x[n - 1] * (s[n] / a[n] - s[n - 1] / a[n - 1])
    return total
