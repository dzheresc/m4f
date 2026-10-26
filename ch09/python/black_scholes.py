"""Black-Scholes prices and the Greek parameters (Section 9.1.2).

S stock price, X strike, r continuously compounded rate, sigma volatility,
T time to expiry. Same norm_cdf as Chapter 8 (Hart 1968 / G. West).
"""
from __future__ import annotations

import math
from dataclasses import dataclass


def norm_cdf(x: float) -> float:
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


def norm_pdf(x: float) -> float:
    return math.exp(-x * x / 2) / 2.506628274631000502


def norm_inv(p: float) -> float:
    """Quantile of the standard normal, by bisection on norm_cdf (0 < p < 1)."""
    lo, hi = -40.0, 40.0
    for _ in range(200):
        mid = (lo + hi) / 2
        if norm_cdf(mid) < p:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2


def d1_d2(S: float, X: float, r: float, sigma: float, T: float) -> tuple[float, float]:
    d1 = (math.log(S / X) + (r + sigma * sigma / 2) * T) / (sigma * math.sqrt(T))
    return d1, d1 - sigma * math.sqrt(T)


def payoff(S: float, X: float, is_call: bool) -> float:
    return max(S - X, 0.0) if is_call else max(X - S, 0.0)


def bs_price(S: float, X: float, r: float, sigma: float, T: float, is_call: bool) -> float:
    """Black-Scholes price; the payoff when T <= 0."""
    if T <= 0:
        return payoff(S, X, is_call)
    d1, d2 = d1_d2(S, X, r, sigma, T)
    if is_call:
        return S * norm_cdf(d1) - X * math.exp(-r * T) * norm_cdf(d2)
    return X * math.exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1)


@dataclass(frozen=True)
class Greeks:
    price: float
    delta: float   # dV/dS
    gamma: float   # d2V/dS2
    theta: float   # dV/dt (per year)
    vega: float    # dV/dsigma
    rho: float     # dV/dr


def bs_greeks(S: float, X: float, r: float, sigma: float, T: float, is_call: bool) -> Greeks:
    """The Greek parameters of a European option in the Black-Scholes model."""
    d1, d2 = d1_d2(S, X, r, sigma, T)
    n1 = norm_pdf(d1)
    disc = X * math.exp(-r * T)
    gamma = n1 / (S * sigma * math.sqrt(T))
    vega = S * n1 * math.sqrt(T)
    if is_call:
        price = S * norm_cdf(d1) - disc * norm_cdf(d2)
        delta = norm_cdf(d1)
        theta = -S * n1 * sigma / (2 * math.sqrt(T)) - r * disc * norm_cdf(d2)
        rho = T * disc * norm_cdf(d2)
    else:
        price = disc * norm_cdf(-d2) - S * norm_cdf(-d1)
        delta = norm_cdf(d1) - 1
        theta = -S * n1 * sigma / (2 * math.sqrt(T)) + r * disc * norm_cdf(-d2)
        rho = -T * disc * norm_cdf(-d2)
    return Greeks(price, delta, gamma, theta, vega, rho)
