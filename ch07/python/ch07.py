#!/usr/bin/env python3
"""Chapter 7 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from binomial import crr_price
from options import (american_parity_bounds, bound_violation, butterfly_gap, call_payoff,
                     european_call_bounds, european_put_bounds, parity_gap, profit, put_payoff)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def price(S0: float, X: float, T: float, is_call: bool, american: bool = False) -> float:
    return crr_price(S0, X, 0.05, 0.2, T, 200, is_call, american)


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 7: Options: General Properties (illustrative numbers, book formulas)")

    line("\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==")
    for ST in (80.0, 100.0, 120.0):
        c, p = call_payoff(ST, 100.0), put_payoff(ST, 100.0)
        line(f"S(T)={f(ST)}: call payoff={f(c)} profit long={f(profit(c, 10.0, 0.05, 1.0))} "
             f"short={f(-profit(c, 10.0, 0.05, 1.0))}; put payoff={f(p)} profit long={f(profit(p, 5.0, 0.05, 1.0))} "
             f"short={f(-profit(p, 5.0, 0.05, 1.0))}")

    line("\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 r=0.05 T=1 ==")
    line(f"S(0) - X e^(-rT) = {f(100.0 - 100.0 * math.exp(-0.05))}")
    for C, P in ((10.45, 5.57), (12.0, 5.0), (9.0, 6.0)):
        gap = parity_gap(C, P, 100.0, 100.0, 0.05, 1.0)
        line(f"C={f(C)} P={f(P)}: gap={f(gap)} arbitrage profit at T={f(abs(gap) * math.exp(0.05))}")
    line(f"Dividend 2 at t=0.5 (PV {f(2 * math.exp(-0.025))}): C - P should be "
         f"{f(-parity_gap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, 2 * math.exp(-0.025)))}")

    line("\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 ==")
    cb, pb = european_call_bounds(100.0, 100.0, 0.05, 1.0), european_put_bounds(100.0, 100.0, 0.05, 1.0)
    line(f"call in {fl(list(cb))}, put in {fl(list(pb))}")
    line(f"Quoted call 3.50: violation {f(bound_violation(3.5, cb))}; quoted put 96.00: violation "
         f"{f(bound_violation(96.0, pb))}; model call {f(price(100.0, 100.0, 1.0, True))}: "
         f"violation {f(bound_violation(price(100.0, 100.0, 1.0, True), cb))}")

    line("\n== 7.3.2 American calls on a non-dividend paying stock (CRR, sigma=0.2, N=200) ==")
    for S0 in (90.0, 100.0, 120.0):
        CE, CA = price(S0, 100.0, 1.0, True), price(S0, 100.0, 1.0, True, True)
        line(f"S(0)={f(S0)}: C_E={f(CE)} C_A={f(CA)} exercise now={f(call_payoff(S0, 100.0))}")

    line("\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) ==")
    for S0 in (90.0, 100.0, 120.0):
        PE, PA = price(S0, 100.0, 1.0, False), price(S0, 100.0, 1.0, False, True)
        CA = price(S0, 100.0, 1.0, True, True)
        lo, hi = american_parity_bounds(S0, 100.0, 0.05, 1.0)
        line(f"S(0)={f(S0)}: P_E={f(PE)} P_A={f(PA)} C_A-P_A={f(CA - PA)} in {fl([lo, hi])}")

    line("\n== 7.4.1 European options: dependence on X and S(0) ==")
    Xs = [90.0, 100.0, 110.0]
    Cs = [price(100.0, X, 1.0, True) for X in Xs]
    line(f"X={fl(Xs)}: C={fl(Cs)} P={fl([price(100.0, X, 1.0, False) for X in Xs])}")
    line(f"C(90)-C(100)={f(Cs[0] - Cs[1])} <= e^(-rT) x 10 = {f(10 * math.exp(-0.05))}; "
         f"butterfly gap={f(butterfly_gap(Cs[0], Cs[1], Cs[2], 90.0, 100.0, 110.0))} (<= 0)")
    line(f"Quotes C=15, 10, 2: butterfly gap={f(butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0))} "
         f"-> write 2 x C(100), buy C(90) and C(110)")
    line(f"S(0)={fl([90.0, 100.0, 110.0])}: C={fl([price(S, 100.0, 1.0, True) for S in (90.0, 100.0, 110.0)])}")

    line("\n== 7.4.2 American options: dependence on T ==")
    Ts = [0.25, 0.5, 1.0, 2.0]
    line(f"T={fl(Ts)}: P_A={fl([price(100.0, 100.0, T, False, True) for T in Ts])}")

    line("\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 ==")
    for S0 in (80.0, 100.0, 120.0):
        C, P = price(S0, 100.0, 1.0, True), price(S0, 100.0, 1.0, False)
        line(f"S(0)={f(S0)}: call {f(C)} time value {f(C - call_payoff(S0, 100.0))}; "
             f"put {f(P)} time value {f(P - put_payoff(S0, 100.0))}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
