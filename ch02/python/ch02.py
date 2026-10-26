#!/usr/bin/env python3
"""Chapter 2 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from interest import (annuity_future_value, annuity_present_value, continuous_effective_rate,
                      continuous_from_periodic, continuous_present_value, continuous_value,
                      effective_rate, loan_installment, periodic_from_continuous,
                      periodic_from_periodic, periodic_present_value, periodic_value,
                      perpetuity_present_value, simple_present_value, simple_rate, simple_value)
from money_market import (coupon_bond_price, implied_rate, money_market_rollover,
                          money_market_value, yield_to_maturity, zero_coupon_price)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)")

    line("\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==")
    for t in (0.5, 1.0, 2.0):
        line(f"P=2000 r=0.08 t={f(t)}: V(t)={f(simple_value(2000.0, 0.08, t))}")
    line(f"Present value of 1000 due at t=0.5, r=0.10: {f(simple_present_value(1000.0, 0.10, 0.5))}")
    line(f"Rate turning 1000 into 1100 in 2 years: {f(simple_rate(1000.0, 1100.0, 2.0))}")

    line("\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==")
    for m in (1, 2, 4, 12, 365):
        line(f"P=1000 r=0.12 m={m}: V(1)={f(periodic_value(1000.0, 0.12, m, 1.0))} "
             f"effective rate={f(effective_rate(0.12, m))}")
    line(f"Present value of 1000 due in 3 years, r=0.10 annual: "
         f"{f(periodic_present_value(1000.0, 0.10, 1, 3.0))}")

    line("\n== 2.1.3 Streams of payments ==")
    line(f"Annuity of 10 x 1000 at 10%: PV={f(annuity_present_value(1000.0, 0.10, 10))} "
         f"FV={f(annuity_future_value(1000.0, 0.10, 10))}")
    line(f"Perpetuity of 100 a year at 5%: PV={f(perpetuity_present_value(100.0, 0.05))}")
    C = loan_installment(100000.0, 0.06 / 12, 300)
    line(f"Loan 100000 over 25 years, monthly at 6%: installment={f(C)} "
         f"balance after 10 years={f(annuity_present_value(C, 0.06 / 12, 180))}")

    line("\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==")
    line(f"P=1000 r=0.12: V(1)={f(continuous_value(1000.0, 0.12, 1.0))} "
         f"effective rate={f(continuous_effective_rate(0.12))}")
    line(f"Present value of 1000 due in 3 years, r=0.10 continuous: "
         f"{f(continuous_present_value(1000.0, 0.10, 3.0))}")

    line("\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==")
    line(f"Growth factor: simple {f(simple_value(1.0, 0.10, 1.0))}, "
         f"m=1 {f(periodic_value(1.0, 0.10, 1, 1.0))}, m=2 {f(periodic_value(1.0, 0.10, 2, 1.0))}, "
         f"m=12 {f(periodic_value(1.0, 0.10, 12, 1.0))}, "
         f"continuous {f(continuous_value(1.0, 0.10, 1.0))}")
    line(f"12% monthly = {f(continuous_from_periodic(0.12, 12))} continuous")
    line(f"10% continuous = {f(periodic_from_continuous(0.10, 1))} annual "
         f"= {f(periodic_from_continuous(0.10, 12))} monthly")
    line(f"12% quarterly = {f(periodic_from_periodic(0.12, 4, 12))} monthly "
         f"= {f(periodic_from_periodic(0.12, 4, 1))} annual")

    line("\n== 2.2.1 Zero-coupon bonds ==")
    line(f"B(0,1)=0.92 implies: annual {f(implied_rate(0.92, 1.0, 1))}, "
         f"semi-annual {f(implied_rate(0.92, 1.0, 2))}, continuous {f(implied_rate(0.92, 1.0))}")
    line(f"Face 100, T=2 at 10%: annual {f(100.0 * zero_coupon_price(0.10, 2.0, 1))}, "
         f"continuous {f(100.0 * zero_coupon_price(0.10, 2.0))}")

    line("\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==")
    for y in (0.12, 0.10, 0.08):
        line(f"yield {f(y)}: price {f(coupon_bond_price(100.0, 10.0, 3, y))}")
    line(f"price 95.0000: yield to maturity {f(yield_to_maturity(95.0, 100.0, 10.0, 3))}")

    line("\n== 2.2.3 Money market account ==")
    line(f"A(0)=100 at 8% continuous: A(1)={f(money_market_value(100.0, 0.08, 1.0))}")
    A1 = money_market_rollover(100.0, [0.98, 0.985, 0.97, 0.99])
    line(f"Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): A(1)={f(A1)} "
         f"continuous rate {f(math.log(A1 / 100.0))}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
