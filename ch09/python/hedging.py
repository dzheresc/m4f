"""Section 9.1: hedging option positions.

Positions are hedged with the stock, the money market and other options. A
Position is a number of options (negative = written) with a given strike,
expiry and type; all options are on the same stock, priced by Black-Scholes
with a common r and sigma.
"""

from collections.abc import Iterable
import dataclasses
import math

import black_scholes


def plain_sum(xs: Iterable[float]) -> float:
    """Left-to-right float sum.

    The builtin sum() is compensated since Python 3.12 and would not match the
    C++ and Kotlin loops bit for bit.
    """
    t = 0.0
    for x in xs:
        t += x
    return t


@dataclasses.dataclass(frozen=True)
class Position:
    qty: float
    strike: float
    expiry: float
    is_call: bool


@dataclasses.dataclass(frozen=True)
class Hedge:
    """Options held, plus shares and money market balance at time 0."""

    options: list[Position]
    shares: float
    money: float


def portfolio_value(
    h: Hedge, s: float, t: float, r: float, sigma: float
) -> float:
    """Value at time t when the stock is at s (money accrues at e^(rt))."""
    v = h.shares * s + h.money * math.exp(r * t)
    for p in h.options:
        v += p.qty * black_scholes.bs_price(
            s, p.strike, r, sigma, p.expiry - t, p.is_call
        )
    return v


def total_greek(
    options: list[Position], s: float, r: float, sigma: float, name: str
) -> float:
    """Sum of qty times the named Greek over the options."""
    return plain_sum(
        p.qty
        * getattr(
            black_scholes.bs_greeks(s, p.strike, r, sigma, p.expiry, p.is_call),
            name,
        )
        for p in options
    )


def premium(options: list[Position], s: float, r: float, sigma: float) -> float:
    """Cash received for the options (negative of their total value)."""
    return -plain_sum(
        p.qty
        * black_scholes.bs_price(s, p.strike, r, sigma, p.expiry, p.is_call)
        for p in options
    )


def delta_hedge(
    options: list[Position], s: float, r: float, sigma: float
) -> Hedge:
    """Section 9.1.1: make the position delta neutral with shares.

    The money market absorbs the premium received and the cost of the shares
    (value 0 at t=0).
    """
    shares = -total_greek(options, s, r, sigma, "delta")
    return Hedge(
        list(options), shares, premium(options, s, r, sigma) - shares * s
    )


def delta_greek_hedge(
    options: list[Position],
    extra: Position,
    s: float,
    r: float,
    sigma: float,
    name: str,
) -> Hedge:
    """Section 9.1.2: neutralise delta and one more Greek (gamma or vega).

    Uses shares and a position in a second option `extra` (its qty is ignored
    and solved for).
    """
    g_extra = getattr(
        black_scholes.bs_greeks(
            s, extra.strike, r, sigma, extra.expiry, extra.is_call
        ),
        name,
    )
    qty = -total_greek(options, s, r, sigma, name) / g_extra
    all_options = list(options) + [
        Position(qty, extra.strike, extra.expiry, extra.is_call)
    ]
    shares = -total_greek(all_options, s, r, sigma, "delta")
    return Hedge(
        all_options, shares, premium(all_options, s, r, sigma) - shares * s
    )


@dataclasses.dataclass(frozen=True)
class RebalanceStep:
    """One rebalancing date.

    Attributes:
        t: Time.
        s: Stock price.
        shares: Shares held after rebalancing.
        money: Money market balance after rebalancing.
        value: Mark-to-market value of the whole hedged position.
    """

    t: float
    s: float
    shares: float
    money: float
    value: float


def rebalance_path(
    options: list[Position],
    prices: list[float],
    dt: float,
    r: float,
    sigma: float,
) -> list[RebalanceStep]:
    """Dynamic delta hedging: re-hedge at each price on the path.

    prices[0] is at t=0, then one price every `dt` years.
    """
    steps: list[RebalanceStep] = []
    h = delta_hedge(options, prices[0], r, sigma)
    shares, money = h.shares, h.money
    steps.append(
        RebalanceStep(
            0.0,
            prices[0],
            shares,
            money,
            portfolio_value(h, prices[0], 0.0, r, sigma),
        )
    )
    for i in range(1, len(prices)):
        t = i * dt
        s = prices[i]
        money = money * math.exp(r * dt)
        shifted = [
            Position(p.qty, p.strike, p.expiry - t, p.is_call) for p in options
        ]
        new_shares = -total_greek(shifted, s, r, sigma, "delta")
        money -= (new_shares - shares) * s
        shares = new_shares
        value = shares * s + money
        for p in options:
            value += p.qty * black_scholes.bs_price(
                s, p.strike, r, sigma, p.expiry - t, p.is_call
            )
        steps.append(RebalanceStep(t, s, shares, money, value))
    return steps
