"""Sections 8.1-8.2: European and American options in the binomial tree model.

The tree has S(n) = S(0) (1+u)^k (1+d)^(n-k) after k up moves in n steps, a
money market growing at r per step, and p* = (r - d) / (u - d). Payoffs are
functions of the final stock price. `s0` is the stock price S(0) at the
root of the tree.
"""

import math
from collections.abc import Callable

Payoff = Callable[[float], float]


def risk_neutral_prob(u: float, d: float, r: float) -> float:
    """p* = (r - d) / (u - d)."""
    return (r - d) / (u - d)


def stock_price(s0: float, u: float, d: float, n: int, k: int) -> float:
    """S(0) (1+u)^k (1+d)^(n-k)."""
    return s0 * (1 + u) ** k * (1 + d) ** (n - k)


# Section 8.1.1: one step.
def one_step_replication(
    s0: float, u: float, d: float, r: float, c_up: float, c_down: float
) -> tuple[float, float, float]:
    """Replicates the payoff c_up / c_down in one step.

    Returns:
        (x, y, price): x shares and y units of the money market (A(0) = 1);
        price = x S(0) + y.
    """
    x = (c_up - c_down) / (s0 * (u - d))
    y = (c_down - x * s0 * (1 + d)) / (1 + r)
    return x, y, x * s0 + y


# Sections 8.1.2-8.1.3: backward induction.
def option_tree(
    s0: float,
    u: float,
    d: float,
    r: float,
    num_steps: int,
    payoff: Payoff,
    american: bool = False,
) -> list[list[float]]:
    """values[n][k], option values at every node.

    European: V(n) = (p* V(n+1, up) + (1-p*) V(n+1, down)) / (1+r); American:
    the larger of that and the payoff from exercising now.
    """
    q = risk_neutral_prob(u, d, r)
    values = [
        [
            payoff(stock_price(s0, u, d, num_steps, k))
            for k in range(num_steps + 1)
        ]
    ]
    for n in range(num_steps - 1, -1, -1):
        later = values[0]
        level = []
        for k in range(n + 1):
            v = (q * later[k + 1] + (1 - q) * later[k]) / (1 + r)
            if american:
                v = max(v, payoff(stock_price(s0, u, d, n, k)))
            level.append(v)
        values.insert(0, level)
    return values


def replicating_position(
    s0: float,
    u: float,
    d: float,
    r: float,
    values: list[list[float]],
    n: int,
    k: int,
) -> tuple[float, float]:
    """(x, y) held over (n, n+1] at node (n, k).

    x is the delta of the option, y is in units of the money market worth
    (1+r)^n.
    """
    s_up = stock_price(s0, u, d, n + 1, k + 1)
    s_down = stock_price(s0, u, d, n + 1, k)
    x = (values[n + 1][k + 1] - values[n + 1][k]) / (s_up - s_down)
    y = (values[n][k] - x * stock_price(s0, u, d, n, k)) / (1 + r) ** n
    return x, y


def binomial_weights(p: float, num_steps: int) -> list[float]:
    """C(N,k) p^k (1-p)^(N-k) for k = 0..N.

    Built in log space, updating log C(N, k) from k to k+1: C(N, k) on its own
    overflows a float near N = 1030 even when the weight is tiny.
    """
    if p <= 0 or p >= 1:
        hit = 0 if p <= 0 else num_steps
        return [1.0 if k == hit else 0.0 for k in range(num_steps + 1)]
    log_p = math.log(p)
    log_q = math.log1p(-p)
    log_c = 0.0
    weights = []
    for k in range(num_steps + 1):
        weights.append(math.exp(log_c + k * log_p + (num_steps - k) * log_q))
        if k < num_steps:
            log_c += math.log(num_steps - k) - math.log(k + 1)
    return weights


def binomial_sum(p: float, num_steps: int, m: int) -> float:
    """P(at least m up moves out of N) = sum_{k >= m} C(N,k) p^k (1-p)^(N-k)."""
    total = 0.0
    for k, w in enumerate(binomial_weights(p, num_steps)):
        if k >= m:
            total += w
    return total


def european_price_sum(
    s0: float, u: float, d: float, r: float, num_steps: int, payoff: Payoff
) -> float:
    """Section 8.1.3: (1+r)^-N sum_k C(N,k) p*^k (1-p*)^(N-k) f(S(N, k))."""
    q = risk_neutral_prob(u, d, r)
    total = 0.0
    for k, w in enumerate(binomial_weights(q, num_steps)):
        total += w * payoff(stock_price(s0, u, d, num_steps, k))
    return total * (1 + r) ** -num_steps


# Section 8.1.4: the Cox-Ross-Rubinstein formula.
def crr_call(
    s0: float, strike: float, u: float, d: float, r: float, num_steps: int
) -> tuple[float, int]:
    """C = S(0) Phi(m; N, p') - X (1+r)^-N Phi(m; N, p*).

    m is the least number of up moves for which S(N) > X and
    p' = p* (1+u) / (1+r).

    Returns:
        (C, m).
    """
    q = risk_neutral_prob(u, d, r)
    m = 0
    while m <= num_steps and stock_price(s0, u, d, num_steps, m) <= strike:
        m += 1
    q_prime = q * (1 + u) / (1 + r)
    price = s0 * binomial_sum(q_prime, num_steps, m) - strike * (
        1 + r
    ) ** -num_steps * binomial_sum(q, num_steps, m)
    return price, m


# Section 8.2: American options.
def early_exercise_nodes(
    s0: float, u: float, d: float, r: float, num_steps: int, payoff: Payoff
) -> list[tuple[int, int]]:
    """Nodes (n, k), n < N, where exercising beats waiting."""
    q = risk_neutral_prob(u, d, r)
    values = option_tree(s0, u, d, r, num_steps, payoff, american=True)
    nodes = []
    for n in range(num_steps):
        for k in range(n + 1):
            wait = (q * values[n + 1][k + 1] + (1 - q) * values[n + 1][k]) / (
                1 + r
            )
            if payoff(stock_price(s0, u, d, n, k)) > wait:
                nodes.append((n, k))
    return nodes
