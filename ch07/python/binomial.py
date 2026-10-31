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

    # With down = 1/up the stock price at node (n, k) is s0 up^(2k - n), so
    # all node prices come from one table of 2N+1 powers instead of two powers
    # per node: prices[2k - n + N].
    prices = [s0 * up ** (j - num_steps) for j in range(2 * num_steps + 1)]
    values = [payoff(prices[2 * k]) for k in range(num_steps + 1)]
    for n in range(num_steps - 1, -1, -1):
        # One row of the backward induction: values[k] from values[k], [k+1].
        held = [
            (q * v_up + (1 - q) * v_down) / growth
            for v_down, v_up in zip(values, values[1:])
        ]
        if american:
            offset = num_steps - n
            held = [
                max(v, payoff(prices[2 * k + offset]))
                for k, v in enumerate(held)
            ]
        values = held
    return values[0]
