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


# Acklam's rational approximation to the normal quantile: central region
# numerator (_A) and denominator (_B), tails (_C, _D).
_A = (
    -3.969683028665376e01,
    2.209460984245205e02,
    -2.759285104469687e02,
    1.383577518672690e02,
    -3.066479806614716e01,
    2.506628277459239e00,
)
_B = (
    -5.447609879822406e01,
    1.615858368580409e02,
    -1.556989798598866e02,
    6.680131188771972e01,
    -1.328068155288572e01,
)
_C = (
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e00,
    -2.549732539343734e00,
    4.374664141464968e00,
    2.938163982698783e00,
)
_D = (
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e00,
    3.754408661907416e00,
)
_P_LOW = 0.02425


def norm_inv(p: float) -> float:
    """Inverse of norm_cdf.

    Acklam's rational approximation (relative error below 1.2e-9) followed by
    one Halley step against norm_cdf itself, which brings it to full double
    precision, about the cost of two norm_cdf calls. p outside (0, 1) returns
    -40 or 40.
    """
    if not p > 0:
        return -40.0
    if not p < 1:
        return 40.0
    a, b, c, d = _A, _B, _C, _D
    if p < _P_LOW or p > 1 - _P_LOW:
        # Tails: q = sqrt(-2 ln(min(p, 1-p))).
        q = math.sqrt(-2 * math.log(p if p < _P_LOW else 1 - p))
        x = (
            ((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]
        ) / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1)
        if p > 1 - _P_LOW:
            x = -x
    else:
        q = p - 0.5
        r = q * q
        x = (
            (
                ((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r
                + a[5]
            )
            * q
            / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1)
        )
    # Halley step on norm_cdf(x) - p.
    e = norm_cdf(x) - p
    u = e * 2.506628274631000502 * math.exp(x * x / 2)
    return x - u / (1 + x * u / 2)


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
    theta0 = -s * n1 * sigma / (2 * math.sqrt(maturity))
    # Each norm_cdf is evaluated once.
    if is_call:
        nd1 = norm_cdf(d1)
        nd2 = norm_cdf(d2)
        price = s * nd1 - disc * nd2
        delta = nd1
        theta = theta0 - r * disc * nd2
        rho = maturity * disc * nd2
    else:
        nmd1 = norm_cdf(-d1)
        nmd2 = norm_cdf(-d2)
        price = disc * nmd2 - s * nmd1
        delta = norm_cdf(d1) - 1
        theta = theta0 + r * disc * nmd2
        rho = -maturity * disc * nmd2
    return Greeks(price, delta, gamma, theta, vega, rho)
