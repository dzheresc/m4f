"""Option prices used to illustrate Sections 7.3-7.5: the Cox-Ross-Rubinstein
tree with N steps of length tau = T/N, u = e^(sigma sqrt(tau)), d = 1/u,
growth e^(r tau) per step (Chapter 8 develops the model itself).
"""
from __future__ import annotations

import math


def crr_price(S0: float, X: float, r: float, sigma: float, T: float, N: int,
              is_call: bool, american: bool = False) -> float:
    tau = T / N
    up = math.exp(sigma * math.sqrt(tau))
    down = 1 / up
    growth = math.exp(r * tau)
    q = (growth - down) / (up - down)

    def payoff(S: float) -> float:
        return max(S - X, 0.0) if is_call else max(X - S, 0.0)

    values = [payoff(S0 * up ** k * down ** (N - k)) for k in range(N + 1)]
    for n in range(N - 1, -1, -1):
        for k in range(n + 1):
            v = (q * values[k + 1] + (1 - q) * values[k]) / growth
            if american:
                v = max(v, payoff(S0 * up ** k * down ** (n - k)))
            values[k] = v
    return values[0]
