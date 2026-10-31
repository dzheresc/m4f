"""Section 11.3: interest rate derivatives on the binomial tree.

Floating payments are the simple one-period rate fixed at (n,k) and paid at
n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.
"""

from collections.abc import Iterable

import rate_tree

Lattice = rate_tree.Lattice


def plain_sum(xs: Iterable[float]) -> float:
    """Left-to-right float sum.

    The builtin sum() is compensated since Python 3.12 and would not match the
    C++ and Kotlin loops bit for bit.
    """
    t = 0.0
    for x in xs:
        t += x
    return t


def floating_rate(rates: Lattice, tau: float, n: int, k: int) -> float:
    """L(n,k) = 1/B(n,n+1;k) - 1."""
    return 1 / rate_tree.one_step_price(rates[n][k], tau) - 1


def bond_option(
    rates: Lattice,
    probs: Lattice,
    tau: float,
    maturity: int,
    expiry: int,
    strike: float,
    is_call: bool,
    american: bool = False,
) -> Lattice:
    """11.3.1: option on a zero-coupon bond.

    Args:
        rates: the short rate lattice.
        probs: the risk-neutral up probabilities.
        tau: the length of a time step.
        maturity: the bond's maturity step.
        expiry: the option's expiry step, at most `maturity`.
        strike: the strike price.
        is_call: True for a call, False for a put.
        american: whether early exercise is allowed.

    Returns:
        The option value lattice for levels 0..expiry.
    """
    bond = rate_tree.bond_lattice(rates, probs, tau, maturity)

    def payoff(b: float) -> float:
        return max(b - strike, 0.0) if is_call else max(strike - b, 0.0)

    def exercise(n: int, k: int) -> float:
        return payoff(bond[n][k])

    terminal = [payoff(bond[expiry][k]) for k in range(expiry + 1)]
    return rate_tree.discount_back(
        rates, probs, tau, terminal, early=exercise if american else None
    )


def swap_rate(bond_prices: list[float]) -> float:
    """11.3.2: the fixed rate K making a swap worthless.

    The swap exchanges fixed K against floating L(n,k), N payments at 1..N on
    notional 1, so K sum B(0,n) = 1 - B(0,N).
    """
    return (1 - bond_prices[-1]) / plain_sum(bond_prices)


def price_payments(
    rates: Lattice,
    probs: Lattice,
    tau: float,
    num_steps: int,
    payment: rate_tree.NodeFn,
) -> Lattice:
    """Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1."""
    return rate_tree.discount_back(
        rates, probs, tau, [0.0] * (num_steps + 1), payment=payment
    )


def payer_swap(
    rates: Lattice,
    probs: Lattice,
    tau: float,
    num_steps: int,
    fixed_rate: float,
) -> Lattice:
    """Receive floating, pay fixed `fixed_rate`."""
    return price_payments(
        rates,
        probs,
        tau,
        num_steps,
        lambda n, k: floating_rate(rates, tau, n, k) - fixed_rate,
    )


def cap(
    rates: Lattice, probs: Lattice, tau: float, num_steps: int, strike: float
) -> Lattice:
    """11.3.3: a cap; each caplet pays max(L(n,k) - K, 0)."""
    return price_payments(
        rates,
        probs,
        tau,
        num_steps,
        lambda n, k: max(floating_rate(rates, tau, n, k) - strike, 0.0),
    )


def floor(
    rates: Lattice, probs: Lattice, tau: float, num_steps: int, strike: float
) -> Lattice:
    """11.3.3: a floor; each floorlet pays max(K - L(n,k), 0)."""
    return price_payments(
        rates,
        probs,
        tau,
        num_steps,
        lambda n, k: max(strike - floating_rate(rates, tau, n, k), 0.0),
    )


def caplet(
    rates: Lattice, probs: Lattice, tau: float, fixing: int, strike: float
) -> float:
    """Value at 0 of the single caplet fixed at `fixing`, paid at fixing + 1.

    Only level `fixing` pays, so this rolls one row back from there instead of
    building the whole price_payments() lattice; the result is the same.
    """
    v = [
        rate_tree.one_step_price(rates[fixing][k], tau)
        * max(floating_rate(rates, tau, fixing, k) - strike, 0.0)
        for k in range(fixing + 1)
    ]
    for n in range(fixing - 1, -1, -1):
        v = [
            rate_tree.one_step_price(r, tau) * (p * v_up + (1 - p) * v_down)
            for r, p, v_down, v_up in zip(rates[n], probs[n], v, v[1:])
        ]
    return v[0]
