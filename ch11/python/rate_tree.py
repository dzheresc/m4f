"""Sections 11.1-11.2: the binomial tree model of the short rate.

Time steps of length tau; node (n,k) is reached after n steps of which k were
"up". A Lattice holds one value per node: L[n][k], k = 0..n. The short rate
r(n,k) applies over [n, n+1], so the one-step bond is
B(n,n+1;k) = e^(-r(n,k) tau) and the money market account grows by e^(r tau)
each step. Risk-neutral probabilities p*(n,k) of an up move may vary by node.
"""

from collections.abc import Callable
import math

Lattice = list[list[float]]
# A value attached to node (n, k).
NodeFn = Callable[[int, int], float]


def rate_lattice(r0: float, up: float, down: float, num_steps: int) -> Lattice:
    """Additive tree: r(n,k) = r0 + k up + (n-k) down.

    Args:
        r0: the short rate at the root.
        up: change of the rate on an up move.
        down: change of the rate on a down move (negative for a fall).
        num_steps: number of time steps N.

    Returns:
        The rate lattice for levels 0..N.
    """
    return [
        [r0 + k * up + (n - k) * down for k in range(n + 1)]
        for n in range(num_steps + 1)
    ]


def constant_probs(p: float, num_steps: int) -> Lattice:
    """The same up probability p at every node of levels 0..N-1."""
    return [[p for _ in range(n + 1)] for n in range(num_steps)]


def one_step_price(r: float, tau: float) -> float:
    """B(n,n+1) = e^(-r tau)."""
    return math.exp(-r * tau)


def money_market(rates: Lattice, tau: float, path: str) -> list[float]:
    """A(0..len(path)) along a path of 'u'/'d' moves.

    A(n+1) = A(n) e^(r(n,k) tau).
    """
    account = [1.0]
    k = 0
    for n, move in enumerate(path):
        account.append(account[-1] * math.exp(rates[n][k] * tau))
        if move == "u":
            k += 1
    return account


def discount_back(
    rates: Lattice,
    probs: Lattice,
    tau: float,
    terminal: list[float],
    payment: NodeFn | None = None,
    early: NodeFn | None = None,
) -> Lattice:
    """Backward induction from level N = len(terminal)-1 down to 0.

    V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
    with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
    is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise).

    Args:
        rates: the short rate lattice.
        probs: the risk-neutral up probabilities.
        tau: the length of a time step.
        terminal: the values at level N.
        payment: optional payment fixed at (n,k) and made at n+1.
        early: optional early exercise value at (n,k).

    Returns:
        The value lattice for levels 0..N.
    """
    num_steps = len(terminal) - 1
    later = list(terminal)
    values: Lattice = [later]  # From level N down to level 0.
    for n in range(num_steps - 1, -1, -1):
        row = []
        for k in range(n + 1):
            p = probs[n][k]
            cont = p * later[k + 1] + (1 - p) * later[k]
            if payment is not None:
                cont += payment(n, k)
            v = one_step_price(rates[n][k], tau) * cont
            if early is not None:
                v = max(v, early(n, k))
            row.append(v)
        values.append(row)
        later = row
    values.reverse()
    return values


def bond_lattice(
    rates: Lattice, probs: Lattice, tau: float, maturity: int
) -> Lattice:
    """11.2: B(n,N;k) for all nodes, from B(N,N) = 1."""
    return discount_back(rates, probs, tau, [1.0] * (maturity + 1))


def bond_prices(
    rates: Lattice, probs: Lattice, tau: float, num_steps: int
) -> list[float]:
    """B(0,1), ..., B(0,N), by forward induction.

    Q(n,k) is the Arrow-Debreu state price, the time-0 value of 1 paid at node
    (n,k): B(0,n+1) = sum_k Q(n,k) e^(-r(n,k) tau), and Q(n+1,.) receives
    Q(n,k) e^(-r(n,k) tau) times p* (up) or 1 - p* (down). One O(N^2) pass
    instead of a backward induction per maturity; equal to
    bond_lattice(...)[0][0] up to rounding.
    """
    out = []
    q = [1.0]
    for n in range(num_steps):
        nxt = [0.0] * (n + 2)
        bond = 0.0
        for k, (q_k, r, p) in enumerate(zip(q, rates[n], probs[n])):
            d = q_k * one_step_price(r, tau)
            bond += d
            nxt[k + 1] += p * d
            nxt[k] += (1 - p) * d
        out.append(bond)
        q = nxt
    return out


def yield_rate(bond_price: float, n: int, maturity: int, tau: float) -> float:
    """y(n,N) = -ln B(n,N) / ((N-n) tau)."""
    return -math.log(bond_price) / ((maturity - n) * tau)


def implied_prob(
    bond: Lattice, rates: Lattice, tau: float, n: int, k: int
) -> float:
    """11.2.1: the p*(n,k) that prices the bond lattice at node (n,k).

    B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k).
    """
    return (bond[n][k] * math.exp(rates[n][k] * tau) - bond[n + 1][k]) / (
        bond[n + 1][k + 1] - bond[n + 1][k]
    )


def implied_probs(bond: Lattice, rates: Lattice, tau: float) -> Lattice:
    """implied_prob() at every node of levels 0..N-2 of the bond lattice."""
    maturity = len(bond) - 1
    return [
        [implied_prob(bond, rates, tau, n, k) for k in range(n + 1)]
        for n in range(maturity - 1)
    ]
