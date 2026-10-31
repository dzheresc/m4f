"""Black-Scholes prices and the Greek parameters (Section 9.1.2).

s stock price, strike X, r continuously compounded rate, sigma volatility,
maturity T time to expiry. Same norm_cdf as Chapter 8 (Hart 1968 / G. West).
"""

import dataclasses
import math


def norm_cdf(x: float) -> float:
    """Standard normal CDF to double precision."""
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
    """Standard normal density."""
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


def d1_d2(
    s: float, strike: float, r: float, sigma: float, maturity: float
) -> tuple[float, float]:
    """(d1, d2) of the Black-Scholes formula."""
    d1 = (math.log(s / strike) + (r + sigma * sigma / 2) * maturity) / (
        sigma * math.sqrt(maturity)
    )
    return d1, d1 - sigma * math.sqrt(maturity)


def payoff(s: float, strike: float, is_call: bool) -> float:
    """Payoff of a call or put at expiry."""
    return max(s - strike, 0.0) if is_call else max(strike - s, 0.0)


def bs_price(
    s: float,
    strike: float,
    r: float,
    sigma: float,
    maturity: float,
    is_call: bool,
) -> float:
    """Black-Scholes price; the payoff when maturity <= 0."""
    if maturity <= 0:
        return payoff(s, strike, is_call)
    d1, d2 = d1_d2(s, strike, r, sigma, maturity)
    if is_call:
        return s * norm_cdf(d1) - strike * math.exp(-r * maturity) * norm_cdf(
            d2
        )
    return strike * math.exp(-r * maturity) * norm_cdf(-d2) - s * norm_cdf(-d1)


@dataclasses.dataclass(frozen=True)
class Greeks:
    """Price and Greek parameters of an option.

    Attributes:
        price: The option price V.
        delta: dV/dS.
        gamma: d2V/dS2.
        theta: dV/dt (per year).
        vega: dV/dsigma.
        rho: dV/dr.
    """

    price: float
    delta: float
    gamma: float
    theta: float
    vega: float
    rho: float


def bs_greeks(
    s: float,
    strike: float,
    r: float,
    sigma: float,
    maturity: float,
    is_call: bool,
) -> Greeks:
    """The Greek parameters of a European option in the Black-Scholes model."""
    d1, d2 = d1_d2(s, strike, r, sigma, maturity)
    n1 = norm_pdf(d1)
    disc = strike * math.exp(-r * maturity)
    gamma = n1 / (s * sigma * math.sqrt(maturity))
    vega = s * n1 * math.sqrt(maturity)
    if is_call:
        price = s * norm_cdf(d1) - disc * norm_cdf(d2)
        delta = norm_cdf(d1)
        theta = -s * n1 * sigma / (
            2 * math.sqrt(maturity)
        ) - r * disc * norm_cdf(d2)
        rho = maturity * disc * norm_cdf(d2)
    else:
        price = disc * norm_cdf(-d2) - s * norm_cdf(-d1)
        delta = norm_cdf(d1) - 1
        theta = -s * n1 * sigma / (
            2 * math.sqrt(maturity)
        ) + r * disc * norm_cdf(-d2)
        rho = -maturity * disc * norm_cdf(-d2)
    return Greeks(price, delta, gamma, theta, vega, rho)
