#!/usr/bin/env python3
"""Chapter 11 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from derivatives import plain_sum, bond_option, cap, caplet, floating_rate, floor, payer_swap, swap_rate
from rate_tree import (bond_lattice, bond_prices, constant_probs, implied_prob, implied_probs, money_market,
                       one_step_price, rate_lattice, yield_rate)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 11: Stochastic Interest Rates (illustrative numbers, book formulas)")

    N, tau = 4, 1.0
    rates = rate_lattice(0.05, 0.01, -0.01, N)
    probs = constant_probs(0.5, N)

    line("\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau=1, N=4 ==")
    for n in range(N + 1):
        line(f"n={n}: r={fl(rates[n][::-1])} B(n,n+1)={fl([one_step_price(r, tau) for r in rates[n][::-1]])}")
    for path in ("uuuu", "udud", "dddd"):
        line(f"Money market account along {path}: A={fl(money_market(rates, tau, path))}")

    line("\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==")
    B0 = bond_prices(rates, probs, tau, N)
    line("B(0,N) for N=1..4: " + fl(B0))
    line("Term structure y(0,N): " + fl([yield_rate(B, 0, m + 1, tau) for m, B in enumerate(B0)]))
    B4 = bond_lattice(rates, probs, tau, N)
    for n in range(N + 1):
        line(f"B(n,4) at n={n}: {fl(B4[n][::-1])}")
    B3 = bond_lattice(rates, probs, tau, 3)
    B2 = bond_lattice(rates, probs, tau, 2)
    line("Term structure after one step: up node y(1,N)=" + fl([yield_rate(B[1][1], 1, m, tau) for m, B in ((2, B2), (3, B3), (4, B4))])
         + " down node y(1,N)=" + fl([yield_rate(B[1][0], 1, m, tau) for m, B in ((2, B2), (3, B3), (4, B4))]))
    line("Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: "
         + fl([(0.5 * B[1][1] + 0.5 * B[1][0]) / B[0][0] for B in (B2, B3, B4)]) + f" = e^(r(0) tau) = {f(math.exp(0.05 * tau))}")

    line("\n== 11.2.1 Risk-neutral probabilities implied by bond prices ==")
    for m, B in ((4, B4), (3, B3), (2, B2)):
        P = implied_probs(B, rates, tau)
        line(f"from B(n,{m}): " + " ".join(f"n={n}:{fl(P[n][::-1])}" for n in range(len(P))))
    probs6 = constant_probs(0.6, N)
    B4_6 = bond_lattice(rates, probs6, tau, N)
    line(f"Bonds priced with p*=0.6: B(0,4)={f(B4_6[0][0])} (vs {f(B4[0][0])}); implied p* at (0,0): {f(implied_prob(B4_6, rates, tau, 0, 0))}")
    wrong = [row[:] for row in B2]
    wrong[0][0] = B2[0][0] - 0.01
    line(f"A two-year bond quoted at {f(wrong[0][0])} instead of {f(B2[0][0])} implies p*={f(implied_prob(wrong, rates, tau, 0, 0))} "
         f"at (0,0), inconsistent with the other bonds: arbitrage")

    line("\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==")
    X = 0.90
    for name, is_call in (("call", True), ("put", False)):
        V = bond_option(rates, probs, tau, N, 2, X, is_call)
        line(f"European {name} X={f(X)}: payoff at n=2 {fl(V[2][::-1])}, price {f(V[0][0])}")
    VA = bond_option(rates, probs, tau, N, 2, X, False, american=True)
    line(f"American put X={f(X)}: price {f(VA[0][0])} = immediate exercise X - B(0,4) = {f(X - B4[0][0])}; "
         f"values at n=1 {fl(VA[1][::-1])} = exercise values {fl([max(X - b, 0.0) for b in B4[1][::-1]])}")
    line("American call X=0.85 on the 4-year bond, expiry n=3: "
         f"{f(bond_option(rates, probs, tau, N, 3, 0.85, True, american=True)[0][0])} vs European "
         f"{f(bond_option(rates, probs, tau, N, 3, 0.85, True)[0][0])}")
    line("Call on the 2-year bond expiring at n=1, X=0.95: " + f(bond_option(rates, probs, tau, 2, 1, 0.95, True)[0][0]))

    line("\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, 4 annual payments ==")
    for n in range(N):
        line(f"n={n}: L={fl([floating_rate(rates, tau, n, k) for k in range(n, -1, -1)])}")
    K = swap_rate(B0)
    line(f"Swap rate K = (1 - B(0,4)) / sum B(0,n) = {f(K)}")
    S = payer_swap(rates, probs, tau, N, K)
    line(f"Payer swap value at K: {f(S[0][0])}; at n=1: {fl(S[1][::-1])}")
    for Kx in (0.04, 0.05, 0.06):
        line(f"Payer swap at K={f(Kx)}: {f(payer_swap(rates, probs, tau, N, Kx)[0][0])}")

    line("\n== 11.3.3 Caps and floors on the same floating rate ==")
    for Kx in (0.04, K, 0.06):
        c, fo, s = cap(rates, probs, tau, N, Kx)[0][0], floor(rates, probs, tau, N, Kx)[0][0], payer_swap(rates, probs, tau, N, Kx)[0][0]
        line(f"K={f(Kx)}: cap={f(c)} floor={f(fo)} cap-floor={f(c - fo)} = payer swap {f(s)}")
    line("Caplets at K=0.05 by fixing date n=0..3: " + fl([caplet(rates, probs, tau, n, 0.05) for n in range(N)])
         + f", sum {f(plain_sum(caplet(rates, probs, tau, n, 0.05) for n in range(N)))} = cap {f(cap(rates, probs, tau, N, 0.05)[0][0])}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
