"""Option prices used to illustrate Sections 7.3-7.5.

The Cox-Ross-Rubinstein tree with N steps of length tau = T/N,
u = e^(sigma sqrt(tau)), d = 1/u, growth e^(r tau) per step (Chapter 8
develops the model itself).
"""

import math


def crr_price(
    s0: float,
    strike: float,
    r: float,
    sigma: float,
    maturity: float,
    num_steps: int,
    is_call: bool,
    american: bool = False,
) -> float:
    """Price of a call or put in the CRR tree, European or American."""
    tau = maturity / num_steps
    up = math.exp(sigma * math.sqrt(tau))
    down = 1 / up
    growth = math.exp(r * tau)
    q = (growth - down) / (up - down)

    def payoff(s: float) -> float:
        return max(s - strike, 0.0) if is_call else max(strike - s, 0.0)

    values = [
        payoff(s0 * up**k * down ** (num_steps - k))
        for k in range(num_steps + 1)
    ]
    for n in range(num_steps - 1, -1, -1):
        for k in range(n + 1):
            v = (q * values[k + 1] + (1 - q) * values[k]) / growth
            if american:
                v = max(v, payoff(s0 * up**k * down ** (n - k)))
            values[k] = v
    return values[0]
