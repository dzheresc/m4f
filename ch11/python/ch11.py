#!/usr/bin/env python3
"""Chapter 11 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import math

import derivatives
import rate_tree

NUM_STEPS = 4
TAU = 1.0


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def binomial_tree_section(out: list[str], rates: rate_tree.Lattice) -> None:
    """Section 11.1."""
    out.append(
        "\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k),"
        " tau=1, N=4 =="
    )
    for n in range(NUM_STEPS + 1):
        row = rates[n][::-1]
        one_step = [rate_tree.one_step_price(r, TAU) for r in row]
        out.append(f"n={n}: r={fmt_list(row)} B(n,n+1)={fmt_list(one_step)}")
    for path in ("uuuu", "udud", "dddd"):
        account = rate_tree.money_market(rates, TAU, path)
        out.append(f"Money market account along {path}: A={fmt_list(account)}")


def bond_pricing_section(
    out: list[str], rates: rate_tree.Lattice, probs: rate_tree.Lattice
) -> None:
    """Sections 11.2 and 11.2.1."""
    out.append("\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==")
    b0 = rate_tree.bond_prices(rates, probs, TAU, NUM_STEPS)
    out.append("B(0,N) for N=1..4: " + fmt_list(b0))
    yields = [rate_tree.yield_rate(b, 0, m + 1, TAU) for m, b in enumerate(b0)]
    out.append("Term structure y(0,N): " + fmt_list(yields))
    b4 = rate_tree.bond_lattice(rates, probs, TAU, NUM_STEPS)
    for n in range(NUM_STEPS + 1):
        out.append(f"B(n,4) at n={n}: {fmt_list(b4[n][::-1])}")
    b3 = rate_tree.bond_lattice(rates, probs, TAU, 3)
    b2 = rate_tree.bond_lattice(rates, probs, TAU, 2)
    lattices = ((2, b2), (3, b3), (4, b4))
    y_up = [rate_tree.yield_rate(b[1][1], 1, m, TAU) for m, b in lattices]
    y_down = [rate_tree.yield_rate(b[1][0], 1, m, TAU) for m, b in lattices]
    out.append(
        f"Term structure after one step: up node y(1,N)={fmt_list(y_up)}"
        f" down node y(1,N)={fmt_list(y_down)}"
    )
    returns = [(0.5 * b[1][1] + 0.5 * b[1][0]) / b[0][0] for b in (b2, b3, b4)]
    out.append(
        "Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: "
        f"{fmt_list(returns)} = e^(r(0) tau) = {fmt(math.exp(0.05 * TAU))}"
    )

    out.append(
        "\n== 11.2.1 Risk-neutral probabilities implied by bond prices =="
    )
    for m, b in ((4, b4), (3, b3), (2, b2)):
        implied = rate_tree.implied_probs(b, rates, TAU)
        levels = " ".join(
            f"n={n}:{fmt_list(implied[n][::-1])}" for n in range(len(implied))
        )
        out.append(f"from B(n,{m}): {levels}")
    probs6 = rate_tree.constant_probs(0.6, NUM_STEPS)
    b4_p6 = rate_tree.bond_lattice(rates, probs6, TAU, NUM_STEPS)
    p6 = rate_tree.implied_prob(b4_p6, rates, TAU, 0, 0)
    out.append(
        f"Bonds priced with p*=0.6: B(0,4)={fmt(b4_p6[0][0])} (vs"
        f" {fmt(b4[0][0])}); implied p* at (0,0): {fmt(p6)}"
    )
    wrong = [row[:] for row in b2]
    wrong[0][0] = b2[0][0] - 0.01
    p_wrong = rate_tree.implied_prob(wrong, rates, TAU, 0, 0)
    out.append(
        f"A two-year bond quoted at {fmt(wrong[0][0])} instead of"
        f" {fmt(b2[0][0])} implies p*={fmt(p_wrong)} at (0,0), inconsistent"
        " with the other bonds: arbitrage"
    )


def bond_options_section(
    out: list[str], rates: rate_tree.Lattice, probs: rate_tree.Lattice
) -> None:
    """Section 11.3.1."""
    out.append("\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==")
    strike = 0.90
    for name, is_call in (("call", True), ("put", False)):
        v = derivatives.bond_option(
            rates, probs, TAU, NUM_STEPS, 2, strike, is_call
        )
        out.append(
            f"European {name} X={fmt(strike)}: payoff at n=2"
            f" {fmt_list(v[2][::-1])}, price {fmt(v[0][0])}"
        )
    american = derivatives.bond_option(
        rates, probs, TAU, NUM_STEPS, 2, strike, False, american=True
    )
    b4 = rate_tree.bond_lattice(rates, probs, TAU, NUM_STEPS)
    exercise = [max(strike - b, 0.0) for b in b4[1][::-1]]
    out.append(
        f"American put X={fmt(strike)}: price {fmt(american[0][0])} = immediate"
        f" exercise X - B(0,4) = {fmt(strike - b4[0][0])}; values at n=1"
        f" {fmt_list(american[1][::-1])} = exercise values {fmt_list(exercise)}"
    )
    american_call = derivatives.bond_option(
        rates, probs, TAU, NUM_STEPS, 3, 0.85, True, american=True
    )[0][0]
    european_call = derivatives.bond_option(
        rates, probs, TAU, NUM_STEPS, 3, 0.85, True
    )[0][0]
    out.append(
        "American call X=0.85 on the 4-year bond, expiry n=3:"
        f" {fmt(american_call)} vs European {fmt(european_call)}"
    )
    short_call = derivatives.bond_option(rates, probs, TAU, 2, 1, 0.95, True)
    out.append(
        "Call on the 2-year bond expiring at n=1, X=0.95: "
        + fmt(short_call[0][0])
    )


def swaps_caps_floors_section(
    out: list[str], rates: rate_tree.Lattice, probs: rate_tree.Lattice
) -> None:
    """Sections 11.3.2 and 11.3.3."""
    out.append(
        "\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1,"
        " 4 annual payments =="
    )
    for n in range(NUM_STEPS):
        floating = [
            derivatives.floating_rate(rates, TAU, n, k)
            for k in range(n, -1, -1)
        ]
        out.append(f"n={n}: L={fmt_list(floating)}")
    b0 = rate_tree.bond_prices(rates, probs, TAU, NUM_STEPS)
    swap_rate = derivatives.swap_rate(b0)
    out.append(f"Swap rate K = (1 - B(0,4)) / sum B(0,n) = {fmt(swap_rate)}")
    swap = derivatives.payer_swap(rates, probs, TAU, NUM_STEPS, swap_rate)
    out.append(
        f"Payer swap value at K: {fmt(swap[0][0])}; at n=1:"
        f" {fmt_list(swap[1][::-1])}"
    )
    for fixed_rate in (0.04, 0.05, 0.06):
        value = derivatives.payer_swap(
            rates, probs, TAU, NUM_STEPS, fixed_rate
        )[0][0]
        out.append(f"Payer swap at K={fmt(fixed_rate)}: {fmt(value)}")

    out.append("\n== 11.3.3 Caps and floors on the same floating rate ==")
    for strike in (0.04, swap_rate, 0.06):
        cap = derivatives.cap(rates, probs, TAU, NUM_STEPS, strike)[0][0]
        floor = derivatives.floor(rates, probs, TAU, NUM_STEPS, strike)[0][0]
        swap_value = derivatives.payer_swap(
            rates, probs, TAU, NUM_STEPS, strike
        )[0][0]
        out.append(
            f"K={fmt(strike)}: cap={fmt(cap)} floor={fmt(floor)}"
            f" cap-floor={fmt(cap - floor)} = payer swap {fmt(swap_value)}"
        )
    caplets = [
        derivatives.caplet(rates, probs, TAU, n, 0.05) for n in range(NUM_STEPS)
    ]
    total_cap = derivatives.cap(rates, probs, TAU, NUM_STEPS, 0.05)[0][0]
    out.append(
        f"Caplets at K=0.05 by fixing date n=0..3: {fmt_list(caplets)}, sum"
        f" {fmt(derivatives.plain_sum(caplets))} = cap {fmt(total_cap)}"
    )


def report() -> str:
    """The full chapter 11 report."""
    out = [
        "Chapter 11: Stochastic Interest Rates (illustrative numbers, book"
        " formulas)"
    ]
    rates = rate_tree.rate_lattice(0.05, 0.01, -0.01, NUM_STEPS)
    probs = rate_tree.constant_probs(0.5, NUM_STEPS)
    binomial_tree_section(out, rates)
    bond_pricing_section(out, rates, probs)
    bond_options_section(out, rates, probs)
    swaps_caps_floors_section(out, rates, probs)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
