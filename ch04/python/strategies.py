"""Section 4.1, stock and money market models: investment strategies.

Prices are given along one path: S[n] (or S[j][n] for several stocks) and A[n]
for n = 0..N. A strategy holds x(n) shares and y(n) units of the money market
over the step from n-1 to n, chosen at time n-1; lists are indexed from 0, so
x[0] is x(1).
"""
from __future__ import annotations

from collections.abc import Sequence


def self_financing_multi(V0: float, S: Sequence[Sequence[float]], A: Sequence[float],
                         x: Sequence[Sequence[float]],
                         div: Sequence[Sequence[float]] | None = None) -> tuple[list[float], list[float]]:
    """Several stocks S[j][n], optional dividends div[j][n] paid at n to the
    holders over (n-1, n]. The money market position y(n+1) is set so that
    rebalancing at n costs exactly V(n):
        sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
        V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).
    Returns (y, V) with y[n-1] = y(n) and V[n] = V(n)."""
    N = len(A) - 1
    V = [V0]
    y: list[float] = []
    for n in range(1, N + 1):
        stock = 0.0
        for j in range(len(S)):
            stock += x[j][n - 1] * S[j][n - 1]
        y.append((V[n - 1] - stock) / A[n - 1])
        value = y[n - 1] * A[n]
        for j in range(len(S)):
            d = div[j][n] if div is not None else 0.0
            value += x[j][n - 1] * (S[j][n] + d)
        V.append(value)
    return y, V


def self_financing(V0: float, S: Sequence[float], A: Sequence[float],
                   x: Sequence[float]) -> tuple[list[float], list[float]]:
    """One stock: y(n) and V(n) of the self-financing strategy with initial
    value V0 and stock positions x."""
    return self_financing_multi(V0, [S], A, [x])


def gains(S: Sequence[float], A: Sequence[float], x: Sequence[float], y: Sequence[float]) -> float:
    """V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1))."""
    g = 0.0
    for n in range(1, len(A)):
        g += x[n - 1] * (S[n] - S[n - 1]) + y[n - 1] * (A[n] - A[n - 1])
    return g


def discounted_gains(S: Sequence[float], A: Sequence[float], x: Sequence[float]) -> float:
    """V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n)."""
    g = 0.0
    for n in range(1, len(A)):
        g += x[n - 1] * (S[n] / A[n] - S[n - 1] / A[n - 1])
    return g
