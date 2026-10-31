#!/usr/bin/env python3
"""Chapter 2 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import math

import interest
import money_market


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def simple_interest(out: list[str]) -> None:
    out.append("\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==")
    for t in (0.5, 1.0, 2.0):
        out.append(
            f"P=2000 r=0.08 t={fmt(t)}: "
            f"V(t)={fmt(interest.simple_value(2000.0, 0.08, t))}"
        )
    out.append(
        "Present value of 1000 due at t=0.5, r=0.10: "
        f"{fmt(interest.simple_present_value(1000.0, 0.10, 0.5))}"
    )
    out.append(
        "Rate turning 1000 into 1100 in 2 years: "
        f"{fmt(interest.simple_rate(1000.0, 1100.0, 2.0))}"
    )


def periodic_compounding(out: list[str]) -> None:
    out.append("\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==")
    for m in (1, 2, 4, 12, 365):
        out.append(
            f"P=1000 r=0.12 m={m}: "
            f"V(1)={fmt(interest.periodic_value(1000.0, 0.12, m, 1.0))} "
            f"effective rate={fmt(interest.effective_rate(0.12, m))}"
        )
    out.append(
        "Present value of 1000 due in 3 years, r=0.10 annual: "
        f"{fmt(interest.periodic_present_value(1000.0, 0.10, 1, 3.0))}"
    )


def streams_of_payments(out: list[str]) -> None:
    out.append("\n== 2.1.3 Streams of payments ==")
    out.append(
        "Annuity of 10 x 1000 at 10%: "
        f"PV={fmt(interest.annuity_present_value(1000.0, 0.10, 10))} "
        f"FV={fmt(interest.annuity_future_value(1000.0, 0.10, 10))}"
    )
    out.append(
        "Perpetuity of 100 a year at 5%: "
        f"PV={fmt(interest.perpetuity_present_value(100.0, 0.05))}"
    )
    installment = interest.loan_installment(100000.0, 0.06 / 12, 300)
    balance = interest.annuity_present_value(installment, 0.06 / 12, 180)
    out.append(
        "Loan 100000 over 25 years, monthly at 6%: "
        f"installment={fmt(installment)} "
        f"balance after 10 years={fmt(balance)}"
    )


def continuous_compounding(out: list[str]) -> None:
    out.append("\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==")
    out.append(
        "P=1000 r=0.12: "
        f"V(1)={fmt(interest.continuous_value(1000.0, 0.12, 1.0))} "
        f"effective rate={fmt(interest.continuous_effective_rate(0.12))}"
    )
    out.append(
        "Present value of 1000 due in 3 years, r=0.10 continuous: "
        f"{fmt(interest.continuous_present_value(1000.0, 0.10, 3.0))}"
    )


def comparing_methods(out: list[str]) -> None:
    out.append("\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==")
    out.append(
        f"Growth factor: simple {fmt(interest.simple_value(1.0, 0.10, 1.0))}, "
        f"m=1 {fmt(interest.periodic_value(1.0, 0.10, 1, 1.0))}, "
        f"m=2 {fmt(interest.periodic_value(1.0, 0.10, 2, 1.0))}, "
        f"m=12 {fmt(interest.periodic_value(1.0, 0.10, 12, 1.0))}, "
        f"continuous {fmt(interest.continuous_value(1.0, 0.10, 1.0))}"
    )
    out.append(
        f"12% monthly = {fmt(interest.continuous_from_periodic(0.12, 12))} "
        "continuous"
    )
    out.append(
        "10% continuous = "
        f"{fmt(interest.periodic_from_continuous(0.10, 1))} annual = "
        f"{fmt(interest.periodic_from_continuous(0.10, 12))} monthly"
    )
    out.append(
        "12% quarterly = "
        f"{fmt(interest.periodic_from_periodic(0.12, 4, 12))} monthly = "
        f"{fmt(interest.periodic_from_periodic(0.12, 4, 1))} annual"
    )


def zero_coupon_bonds(out: list[str]) -> None:
    out.append("\n== 2.2.1 Zero-coupon bonds ==")
    out.append(
        "B(0,1)=0.92 implies: "
        f"annual {fmt(money_market.implied_rate(0.92, 1.0, 1))}, "
        f"semi-annual {fmt(money_market.implied_rate(0.92, 1.0, 2))}, "
        f"continuous {fmt(money_market.implied_rate(0.92, 1.0))}"
    )
    out.append(
        "Face 100, T=2 at 10%: "
        f"annual {fmt(100.0 * money_market.zero_coupon_price(0.10, 2.0, 1))}, "
        f"continuous {fmt(100.0 * money_market.zero_coupon_price(0.10, 2.0))}"
    )


def coupon_bonds(out: list[str]) -> None:
    out.append("\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==")
    for y in (0.12, 0.10, 0.08):
        price = money_market.coupon_bond_price(100.0, 10.0, 3, y)
        out.append(f"yield {fmt(y)}: price {fmt(price)}")
    ytm = money_market.yield_to_maturity(95.0, 100.0, 10.0, 3)
    out.append(f"price 95.0000: yield to maturity {fmt(ytm)}")


def money_market_account(out: list[str]) -> None:
    out.append("\n== 2.2.3 Money market account ==")
    out.append(
        "A(0)=100 at 8% continuous: "
        f"A(1)={fmt(money_market.money_market_value(100.0, 0.08, 1.0))}"
    )
    a1 = money_market.money_market_rollover(100.0, [0.98, 0.985, 0.97, 0.99])
    out.append(
        "Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): "
        f"A(1)={fmt(a1)} continuous rate {fmt(math.log(a1 / 100.0))}"
    )


def report() -> str:
    """Returns the chapter's worked examples as text."""
    out = ["Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)"]
    simple_interest(out)
    periodic_compounding(out)
    streams_of_payments(out)
    continuous_compounding(out)
    comparing_methods(out)
    zero_coupon_bonds(out)
    coupon_bonds(out)
    money_market_account(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
