#!/usr/bin/env python3
"""Chapter 10 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import math

import bonds
import term_structure

Y0 = 0.06
V0 = 100000.0


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def single_bonds_section(out: list[str]) -> None:
    """Section 10.1.1."""
    out.append(
        "\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon,"
        " 4 years, y=0.06 continuous =="
    )
    bond = bonds.coupon_bond(100.0, 5.0, 4)
    out.append(f"Price at y=0.06: {fmt(bonds.price(bond, Y0))}")
    out.append(
        "Yield jumps right after purchase; value of the investment at"
        " horizon t (coupons reinvested):"
    )
    for y in (0.05, 0.06, 0.07):
        out.append(
            f"y={fmt(y)}: "
            + " ".join(
                f"t={t}:{fmt(bonds.investment_value(bond, y, float(t)))}"
                for t in (1, 2, 3, 4)
            )
        )
    out.append("Annualised log return over [0,t]:")
    for y in (0.05, 0.06, 0.07):
        out.append(
            f"y={fmt(y)}: "
            + " ".join(
                f"t={t}:{fmt(bonds.log_return(bond, Y0, y, float(t)))}"
                for t in (1, 2, 3, 4)
            )
        )


def duration_section(out: list[str]) -> None:
    """Section 10.1.2."""
    out.append(
        "\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P =="
    )
    bond = bonds.coupon_bond(100.0, 5.0, 4)
    d = bonds.duration(bond, Y0)
    zero = bonds.duration(bonds.zero_coupon(100.0, 4.0), Y0)
    out.append(f"Coupon bond: D={fmt(d)} years; zero-coupon 4y: D={fmt(zero)}")
    h = 1e-4
    slope = (bonds.price(bond, Y0 + h) - bonds.price(bond, Y0 - h)) / (2 * h)
    out.append(
        f"dP/dy by finite difference: {fmt(slope)}, "
        f"-D P = {fmt(-d * bonds.price(bond, Y0))}"
    )
    out.append(
        "Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first"
        " order): "
        + fmt_list(
            [bonds.investment_value(bond, y, d) for y in (0.05, 0.06, 0.07)]
        )
    )
    for name, b in (
        ("5y 4%", bonds.coupon_bond(100.0, 4.0, 5)),
        ("10y 8%", bonds.coupon_bond(100.0, 8.0, 10)),
        ("10y zero", bonds.zero_coupon(100.0, 10.0)),
    ):
        out.append(
            f"{name}: price={fmt(bonds.price(b, Y0))} "
            f"D={fmt(bonds.duration(b, Y0))} at y=0.06; "
            f"D at y=0.08: {fmt(bonds.duration(b, 0.08))}"
        )


def portfolios_section(
    out: list[str], bond_a: bonds.Bond, bond_b: bonds.Bond
) -> None:
    """Section 10.1.3."""
    out.append(
        "\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5"
        " years, y=0.06 =="
    )
    d_a, d_b = bonds.duration(bond_a, Y0), bonds.duration(bond_b, Y0)
    w_a, w_b = bonds.immunising_weights(d_a, d_b, 5.0)
    out.append(
        f"D_A={fmt(d_a)} D_B={fmt(d_b)}: weights w_A={fmt(w_a)} w_B={fmt(w_b)}"
        f" give D={fmt(w_a * d_a + w_b * d_b)}"
    )
    p = bonds.immunise(V0, bond_a, bond_b, Y0, 0.0, 5.0)
    out.append(f"Invest {fmt(V0)}: {fmt(p.q1)} of A and {fmt(p.q2)} of B")
    target = V0 * math.exp(Y0 * 5.0)
    out.append(f"Target at t=5 if y stays 0.06: {fmt(target)}")
    out.append(
        "Value at t=5 after an immediate jump of y (coupons reinvested at the"
        " new yield):"
    )
    for y in (0.04, 0.05, 0.06, 0.07, 0.08):
        v = p.q1 * bonds.investment_value(
            bond_a, y, 5.0
        ) + p.q2 * bonds.investment_value(bond_b, y, 5.0)
        b_alone = (
            V0
            / bonds.price(bond_b, Y0)
            * bonds.investment_value(bond_b, y, 5.0)
        )
        a_alone = (
            V0
            / bonds.price(bond_a, Y0)
            * bonds.investment_value(bond_a, y, 5.0)
        )
        out.append(
            f"y={fmt(y)}: portfolio {fmt(v)} (shortfall {fmt(target - v)}), "
            f"B alone {fmt(b_alone)}, A alone {fmt(a_alone)}"
        )


def dynamic_hedging_section(
    out: list[str], bond_a: bonds.Bond, bond_b: bonds.Bond
) -> None:
    """Section 10.1.4."""
    out.append(
        "\n== 10.1.4 Dynamic hedging: re-immunise every half year along a"
        " yield path =="
    )
    path = [
        (0.0, 0.06),
        (0.5, 0.065),
        (1.0, 0.07),
        (1.5, 0.06),
        (2.0, 0.05),
        (2.5, 0.055),
        (3.0, 0.065),
        (3.5, 0.07),
        (4.0, 0.075),
        (4.5, 0.07),
        (5.0, 0.065),
    ]
    for st in bonds.dynamic_hedge(V0, bond_a, bond_b, 5.0, path):
        out.append(
            f"t={fmt(st.t)} y={fmt(st.y)}: value={fmt(st.value)} "
            f"q_A={fmt(st.q1)} q_B={fmt(st.q2)} D={fmt(st.duration)}"
        )
    fixed = bonds.dynamic_hedge(V0, bond_a, bond_b, 5.0, path, rebalance=False)
    rebalanced = bonds.dynamic_hedge(V0, bond_a, bond_b, 5.0, path)
    out.append(
        f"At t=5: rebalanced {fmt(rebalanced[-1].value)}, "
        f"not rebalanced {fmt(fixed[-1].value)}, "
        f"target {fmt(V0 * math.exp(Y0 * 5.0))}"
    )


def term_structure_section(
    out: list[str], maturities: list[float], prices: list[float]
) -> None:
    """Sections 10.2.1 and 10.2.2."""
    out.append("\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==")
    forwards = term_structure.forward_rates(maturities, prices)
    for t, b, f in zip(maturities, prices, forwards):
        y = term_structure.yield_rate(b, t)
        out.append(
            f"T={fmt(t)}: B={fmt(b)} y(0,T)={fmt(y)} f(0,T-0.5,T)={fmt(f)}"
        )
    rebuilt = term_structure.prices_from_forward_rates(maturities, forwards)
    out.append(f"Prices rebuilt from the forward rates: {fmt_list(rebuilt)}")
    f13 = term_structure.forward_rate(prices[1], 1.0, prices[5], 3.0)
    out.append(
        f"f(0,1,3)={fmt(f13)} = average of the half-year forwards "
        f"{fmt(term_structure.plain_sum(forwards[2:6]) / 4)}"
    )
    out.append(
        "Implied B(1,T) = B(0,T)/B(0,1): "
        + " ".join(
            f"T={fmt(t)}:{fmt(term_structure.implied_price(prices[1], b))}"
            for t, b in zip(maturities[2:], prices[2:])
        )
    )
    f12 = term_structure.forward_rate(prices[1], 1.0, prices[3], 2.0)
    out.append(
        f"FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)={fmt(f12)}: "
        f"buy 1000 one-year bonds (cost {fmt(1000.0 * prices[1])}), "
        f"sell {fmt(1000.0 * prices[1] / prices[3])} two-year bonds; "
        f"repay {fmt(1000.0 * math.exp(f12))}"
    )
    flow_times = [1.0, 2.0, 3.0]
    flow_amounts = [6.0, 6.0, 106.0]
    p = term_structure.coupon_bond_price(
        flow_times, flow_amounts, maturities, prices
    )
    ytm = term_structure.yield_to_maturity(p, flow_times, flow_amounts)
    out.append(
        f"3-year 6% coupon bond under the term structure: price {fmt(p)}, "
        f"yield to maturity {fmt(ytm)} "
        f"(between y(0,1)={fmt(term_structure.yield_rate(prices[1], 1.0))} "
        f"and y(0,3)={fmt(term_structure.yield_rate(prices[5], 3.0))})"
    )

    out.append(
        "\n== 10.2.2 Money market account: rolling over half-year bonds at the"
        " forward rates =="
    )
    account = term_structure.money_market_account(maturities, prices)
    for t, a, b in zip(maturities, account, prices):
        out.append(f"A({fmt(t)})={fmt(a)} = 1/B(0,T)={fmt(1 / b)}")
    growth = math.exp(term_structure.plain_sum(f * 0.5 for f in forwards))
    out.append(
        f"Short rates realised = forward rates: {fmt_list(forwards)}; "
        f"A(3)=exp(sum f dt)={fmt(growth)}"
    )
    out.append(
        f"1 unit at t=0 in the 3-year bond: {fmt(1 / prices[5])} at t=3, "
        f"same as the money market account {fmt(account[5])}"
    )


def report() -> str:
    """The full chapter 10 report."""
    out = [
        "Chapter 10: Variable Interest Rates (illustrative numbers, book"
        " formulas)"
    ]
    single_bonds_section(out)
    duration_section(out)
    bond_a = bonds.coupon_bond(100.0, 4.0, 5)
    bond_b = bonds.coupon_bond(100.0, 8.0, 10)
    portfolios_section(out, bond_a, bond_b)
    dynamic_hedging_section(out, bond_a, bond_b)
    term_structure_section(
        out,
        [0.5, 1.0, 1.5, 2.0, 2.5, 3.0],
        [0.978, 0.953, 0.926, 0.898, 0.870, 0.842],
    )
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
