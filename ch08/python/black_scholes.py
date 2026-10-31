"""Section 8.3: the Black-Scholes formula and the CRR tree that converges to it.

The Cox-Ross-Rubinstein tree has 1+u = e^(sigma sqrt(tau)),
1+d = e^(-sigma sqrt(tau)), 1+r = e^(r tau), tau = T/N. r is the continuously
compounded rate, `s` the stock price now, `strike` the strike X, `sigma` the
volatility and `maturity` the time T to expiry in years.
"""

import math

import binomial_pricing


def norm_cdf(x: float) -> float:
    """Standard normal CDF to double precision.

    Hart 1968, as given by G. West, "Better approximations to cumulative normal
    functions".
    """
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


def d1_d2(
    s: float, strike: float, r: float, sigma: float, maturity: float
) -> tuple[float, float]:
    """(d1, d2) of the Black-Scholes formula."""
    d1 = (math.log(s / strike) + (r + sigma * sigma / 2) * maturity) / (
        sigma * math.sqrt(maturity)
    )
    return d1, d1 - sigma * math.sqrt(maturity)


def bs_call(
    s: float, strike: float, r: float, sigma: float, maturity: float
) -> float:
    """C = S N(d1) - X e^(-rT) N(d2)."""
    d1, d2 = d1_d2(s, strike, r, sigma, maturity)
    return s * norm_cdf(d1) - strike * math.exp(-r * maturity) * norm_cdf(d2)


def bs_put(
    s: float, strike: float, r: float, sigma: float, maturity: float
) -> float:
    """P = X e^(-rT) N(-d2) - S N(-d1)."""
    d1, d2 = d1_d2(s, strike, r, sigma, maturity)
    return strike * math.exp(-r * maturity) * norm_cdf(-d2) - s * norm_cdf(-d1)


def bs_call_delta(
    s: float, strike: float, r: float, sigma: float, maturity: float
) -> float:
    """Shares in the replicating portfolio: N(d1)."""
    return norm_cdf(d1_d2(s, strike, r, sigma, maturity)[0])


def crr_tree_price(
    s: float,
    strike: float,
    r: float,
    sigma: float,
    maturity: float,
    num_steps: int,
    is_call: bool,
    american: bool = False,
) -> float:
    """Price in the CRR tree with num_steps steps up to maturity."""
    tau = maturity / num_steps
    u = math.exp(sigma * math.sqrt(tau)) - 1
    d = math.exp(-sigma * math.sqrt(tau)) - 1
    r_step = math.exp(r * tau) - 1

    def payoff(x: float) -> float:
        return max(x - strike, 0.0) if is_call else max(strike - x, 0.0)

    # The same backward induction as option_tree, keeping one row instead of
    # the whole tree.
    q = binomial_pricing.risk_neutral_prob(u, d, r_step)
    price = binomial_pricing.stock_price_table(s, u, d, num_steps)
    values = [payoff(price(num_steps, k)) for k in range(num_steps + 1)]
    for n in range(num_steps - 1, -1, -1):
        values = [
            (q * v_up + (1 - q) * v_down) / (1 + r_step)
            for v_down, v_up in zip(values, values[1:])
        ]
        if american:
            values = [max(v, payoff(price(n, k))) for k, v in enumerate(values)]
    return values[0]
