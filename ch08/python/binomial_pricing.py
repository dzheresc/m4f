"""Sections 8.1-8.2: European and American options in the binomial tree model.

The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a
money market growing at r per step, and p* = (r - d) / (u - d). Payoffs are
functions of the final stock price.
"""
from __future__ import annotations

from collections.abc import Callable

Payoff = Callable[[float], float]


def risk_neutral_prob(u: float, d: float, r: float) -> float:
    return (r - d) / (u - d)


def stock_price(S0: float, u: float, d: float, n: int, k: int) -> float:
    return S0 * (1 + u) ** k * (1 + d) ** (n - k)


# 8.1.1 One step
def one_step_replication(S0: float, u: float, d: float, r: float, Cu: float,
                         Cd: float) -> tuple[float, float, float]:
    """(x, y, price): x shares and y units of the money market (A(0) = 1)
    replicating the payoff Cu / Cd; price = x S(0) + y."""
    x = (Cu - Cd) / (S0 * (u - d))
    y = (Cd - x * S0 * (1 + d)) / (1 + r)
    return x, y, x * S0 + y


# 8.1.2-8.1.3 Backward induction
def option_tree(S0: float, u: float, d: float, r: float, N: int, payoff: Payoff,
                american: bool = False) -> list[list[float]]:
    """V[n][k], option values at every node. European: V(n) = (p* V(n+1, up) +
    (1-p*) V(n+1, down)) / (1+r); American: the larger of that and the payoff
    from exercising now."""
    q = risk_neutral_prob(u, d, r)
    V = [[payoff(stock_price(S0, u, d, N, k)) for k in range(N + 1)]]
    for n in range(N - 1, -1, -1):
        nxt = V[0]
        level = []
        for k in range(n + 1):
            v = (q * nxt[k + 1] + (1 - q) * nxt[k]) / (1 + r)
            if american:
                v = max(v, payoff(stock_price(S0, u, d, n, k)))
            level.append(v)
        V.insert(0, level)
    return V


def replicating_position(S0: float, u: float, d: float, r: float, V: list[list[float]],
                         n: int, k: int) -> tuple[float, float]:
    """(x, y) held over (n, n+1] at node (n, k): x = delta of the option, y in
    units of the money market worth (1+r)^n."""
    Su, Sd = stock_price(S0, u, d, n + 1, k + 1), stock_price(S0, u, d, n + 1, k)
    x = (V[n + 1][k + 1] - V[n + 1][k]) / (Su - Sd)
    y = (V[n][k] - x * stock_price(S0, u, d, n, k)) / (1 + r) ** n
    return x, y


def binomial_sum(p: float, N: int, m: int) -> float:
    """P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k)."""
    total = 0.0
    c = 1.0
    for k in range(N + 1):
        if k >= m:
            total += c * p ** k * (1 - p) ** (N - k)
        c = c * (N - k) / (k + 1)
    return total


def european_price_sum(S0: float, u: float, d: float, r: float, N: int, payoff: Payoff) -> float:
    """8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k))."""
    q = risk_neutral_prob(u, d, r)
    total = 0.0
    c = 1.0
    for k in range(N + 1):
        total += c * q ** k * (1 - q) ** (N - k) * payoff(stock_price(S0, u, d, N, k))
        c = c * (N - k) / (k + 1)
    return total * (1 + r) ** -N


# 8.1.4 Cox-Ross-Rubinstein formula
def crr_call(S0: float, X: float, u: float, d: float, r: float, N: int) -> tuple[float, int]:
    """C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*), with m the least number
    of up moves for which S(N) > X and p' = p* (1+u) / (1+r). Returns (C, m)."""
    q = risk_neutral_prob(u, d, r)
    m = 0
    while m <= N and stock_price(S0, u, d, N, m) <= X:
        m += 1
    qq = q * (1 + u) / (1 + r)
    return S0 * binomial_sum(qq, N, m) - X * (1 + r) ** -N * binomial_sum(q, N, m), m


# 8.2 American options
def early_exercise_nodes(S0: float, u: float, d: float, r: float, N: int,
                         payoff: Payoff) -> list[tuple[int, int]]:
    """Nodes (n, k), n < N, where exercising beats waiting."""
    q = risk_neutral_prob(u, d, r)
    V = option_tree(S0, u, d, r, N, payoff, american=True)
    nodes = []
    for n in range(N):
        for k in range(n + 1):
            wait = (q * V[n + 1][k + 1] + (1 - q) * V[n + 1][k]) / (1 + r)
            if payoff(stock_price(S0, u, d, n, k)) > wait:
                nodes.append((n, k))
    return nodes
