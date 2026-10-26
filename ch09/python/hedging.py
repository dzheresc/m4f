"""Section 9.1: hedging option positions with the stock, the money market and
other options. A Position is a number of options (negative = written) with a
given strike, expiry and type; all options are on the same stock, priced by
Black-Scholes with a common r and sigma.
"""
from __future__ import annotations

import math
from dataclasses import dataclass

from black_scholes import bs_greeks, bs_price


def plain_sum(xs) -> float:
    """Left-to-right float sum (builtin sum() is compensated since Python 3.12
    and would not match the C++ and Kotlin loops bit for bit)."""
    t = 0.0
    for x in xs:
        t += x
    return t


@dataclass(frozen=True)
class Position:
    qty: float
    X: float
    T: float
    is_call: bool


@dataclass(frozen=True)
class Hedge:
    """Options held, plus shares and money market balance at time 0."""
    options: list[Position]
    shares: float
    money: float


def portfolio_value(h: Hedge, S: float, t: float, r: float, sigma: float) -> float:
    """Value at time t when the stock is at S (money accrues at e^(rt))."""
    v = h.shares * S + h.money * math.exp(r * t)
    for p in h.options:
        v += p.qty * bs_price(S, p.X, r, sigma, p.T - t, p.is_call)
    return v


def total_greek(options: list[Position], S: float, r: float, sigma: float, name: str) -> float:
    return plain_sum(p.qty * getattr(bs_greeks(S, p.X, r, sigma, p.T, p.is_call), name) for p in options)


def delta_hedge(options: list[Position], S: float, r: float, sigma: float) -> Hedge:
    """9.1.1: make the position delta neutral with shares; the money market
    absorbs the premium received and the cost of the shares (value 0 at t=0)."""
    shares = -total_greek(options, S, r, sigma, "delta")
    premium = -plain_sum(p.qty * bs_price(S, p.X, r, sigma, p.T, p.is_call) for p in options)
    return Hedge(list(options), shares, premium - shares * S)


def delta_greek_hedge(options: list[Position], extra: Position, S: float, r: float, sigma: float,
                      name: str) -> Hedge:
    """9.1.2: neutralise delta and one more Greek (gamma or vega) with shares
    and a position in a second option `extra` (its qty is ignored and solved for)."""
    g_extra = getattr(bs_greeks(S, extra.X, r, sigma, extra.T, extra.is_call), name)
    qty = -total_greek(options, S, r, sigma, name) / g_extra
    all_opts = list(options) + [Position(qty, extra.X, extra.T, extra.is_call)]
    shares = -total_greek(all_opts, S, r, sigma, "delta")
    premium = -plain_sum(p.qty * bs_price(S, p.X, r, sigma, p.T, p.is_call) for p in all_opts)
    return Hedge(all_opts, shares, premium - shares * S)


@dataclass(frozen=True)
class RebalanceStep:
    t: float
    S: float
    shares: float
    money: float
    value: float  # mark-to-market value of the whole hedged position


def rebalance_path(options: list[Position], prices: list[float], dt: float, r: float,
                   sigma: float) -> list[RebalanceStep]:
    """Dynamic delta hedging: re-hedge at each price on the path (prices[0] at t=0)."""
    steps: list[RebalanceStep] = []
    h = delta_hedge(options, prices[0], r, sigma)
    shares, money = h.shares, h.money
    steps.append(RebalanceStep(0.0, prices[0], shares, money, portfolio_value(h, prices[0], 0.0, r, sigma)))
    for i in range(1, len(prices)):
        t = i * dt
        S = prices[i]
        money = money * math.exp(r * dt)
        shifted = [Position(p.qty, p.X, p.T - t, p.is_call) for p in options]
        new_shares = -total_greek(shifted, S, r, sigma, "delta")
        money -= (new_shares - shares) * S
        shares = new_shares
        value = shares * S + money
        for p in options:
            value += p.qty * bs_price(S, p.X, r, sigma, p.T - t, p.is_call)
        steps.append(RebalanceStep(t, S, shares, money, value))
    return steps
