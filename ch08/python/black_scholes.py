"""Section 8.3: the Black-Scholes formula, and the Cox-Ross-Rubinstein tree that
converges to it (1+u = e^(sigma sqrt(tau)), 1+d = e^(-sigma sqrt(tau)),
1+r = e^(r tau), tau = T/N). r is the continuously compounded rate.
"""
from __future__ import annotations

import math

from binomial_pricing import option_tree


def norm_cdf(x: float) -> float:
    """Standard normal CDF to double precision (Hart 1968, as given by
    G. West, "Better approximations to cumulative normal functions")."""
    a = abs(x)
    if a > 37.0:
        c = 0.0
    else:
        e = math.exp(-a * a / 2)
        if a < 7.07106781186547:
            b = 3.52624965998911e-02 * a + 0.700383064443688
            b = b * a + 6.37396220353165
            b = b * a + 33.912866078383
            b = b * a + 112.079291497871
            b = b * a + 221.213596169931
            b = b * a + 220.206867912376
            c = e * b
            b = 8.83883476483184e-02 * a + 1.75566716318264
            b = b * a + 16.064177579207
            b = b * a + 86.7807322029461
            b = b * a + 296.564248779674
            b = b * a + 637.333633378831
            b = b * a + 793.826512519948
            b = b * a + 440.413735824752
            c = c / b
        else:
            b = a + 0.65
            b = a + 4 / b
            b = a + 3 / b
            b = a + 2 / b
            b = a + 1 / b
            c = e / b / 2.506628274631
    return 1 - c if x > 0 else c


def d1_d2(S: float, X: float, r: float, sigma: float, T: float) -> tuple[float, float]:
    d1 = (math.log(S / X) + (r + sigma * sigma / 2) * T) / (sigma * math.sqrt(T))
    return d1, d1 - sigma * math.sqrt(T)


def bs_call(S: float, X: float, r: float, sigma: float, T: float) -> float:
    """C = S N(d1) - X e^(-rT) N(d2)"""
    d1, d2 = d1_d2(S, X, r, sigma, T)
    return S * norm_cdf(d1) - X * math.exp(-r * T) * norm_cdf(d2)


def bs_put(S: float, X: float, r: float, sigma: float, T: float) -> float:
    """P = X e^(-rT) N(-d2) - S N(-d1)"""
    d1, d2 = d1_d2(S, X, r, sigma, T)
    return X * math.exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1)


def bs_call_delta(S: float, X: float, r: float, sigma: float, T: float) -> float:
    """Shares in the replicating portfolio: N(d1)."""
    return norm_cdf(d1_d2(S, X, r, sigma, T)[0])


def crr_tree_price(S: float, X: float, r: float, sigma: float, T: float, N: int, is_call: bool,
                   american: bool = False) -> float:
    tau = T / N
    u = math.exp(sigma * math.sqrt(tau)) - 1
    d = math.exp(-sigma * math.sqrt(tau)) - 1
    rr = math.exp(r * tau) - 1
    payoff = (lambda s: max(s - X, 0.0)) if is_call else (lambda s: max(X - s, 0.0))
    return option_tree(S, u, d, rr, N, payoff, american)[0][0]
