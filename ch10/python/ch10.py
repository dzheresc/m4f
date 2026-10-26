#!/usr/bin/env python3
"""Chapter 10 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from bonds import (coupon_bond, duration, dynamic_hedge, immunise, immunising_weights, investment_value, log_return,
                   price, value, zero_coupon)
from term_structure import (plain_sum, coupon_bond_price, forward_rate, forward_rates, implied_price, money_market_account,
                            prices_from_forward_rates, yield_rate, yield_to_maturity)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 10: Variable Interest Rates (illustrative numbers, book formulas)")

    line("\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 years, y=0.06 continuous ==")
    bond = coupon_bond(100.0, 5.0, 4)
    y0 = 0.06
    line(f"Price at y=0.06: {f(price(bond, y0))}")
    line("Yield jumps right after purchase; value of the investment at horizon t (coupons reinvested):")
    for y in (0.05, 0.06, 0.07):
        line(f"y={f(y)}: " + " ".join(f"t={t}:{f(investment_value(bond, y, float(t)))}" for t in (1, 2, 3, 4)))
    line("Annualised log return over [0,t]:")
    for y in (0.05, 0.06, 0.07):
        line(f"y={f(y)}: " + " ".join(f"t={t}:{f(log_return(bond, y0, y, float(t)))}" for t in (1, 2, 3, 4)))

    line("\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P ==")
    D = duration(bond, y0)
    line(f"Coupon bond: D={f(D)} years; zero-coupon 4y: D={f(duration(zero_coupon(100.0, 4.0), y0))}")
    h = 1e-4
    line(f"dP/dy by finite difference: {f((price(bond, y0 + h) - price(bond, y0 - h)) / (2 * h))}, "
         f"-D P = {f(-D * price(bond, y0))}")
    line(f"Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first order): "
         + fl([investment_value(bond, y, D) for y in (0.05, 0.06, 0.07)]))
    for name, b in (("5y 4%", coupon_bond(100.0, 4.0, 5)), ("10y 8%", coupon_bond(100.0, 8.0, 10)),
                    ("10y zero", zero_coupon(100.0, 10.0))):
        line(f"{name}: price={f(price(b, y0))} D={f(duration(b, y0))} at y=0.06; D at y=0.08: {f(duration(b, 0.08))}")

    line("\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 years, y=0.06 ==")
    A = coupon_bond(100.0, 4.0, 5)
    B = coupon_bond(100.0, 8.0, 10)
    DA, DB = duration(A, y0), duration(B, y0)
    wA, wB = immunising_weights(DA, DB, 5.0)
    line(f"D_A={f(DA)} D_B={f(DB)}: weights w_A={f(wA)} w_B={f(wB)} give D={f(wA * DA + wB * DB)}")
    V0 = 100000.0
    p = immunise(V0, A, B, y0, 0.0, 5.0)
    line(f"Invest {f(V0)}: {f(p.q1)} of A and {f(p.q2)} of B")
    target = V0 * math.exp(y0 * 5.0)
    line(f"Target at t=5 if y stays 0.06: {f(target)}")
    line("Value at t=5 after an immediate jump of y (coupons reinvested at the new yield):")
    for y in (0.04, 0.05, 0.06, 0.07, 0.08):
        v = p.q1 * investment_value(A, y, 5.0) + p.q2 * investment_value(B, y, 5.0)
        line(f"y={f(y)}: portfolio {f(v)} (shortfall {f(target - v)}), B alone {f(V0 / price(B, y0) * investment_value(B, y, 5.0))}, "
             f"A alone {f(V0 / price(A, y0) * investment_value(A, y, 5.0))}")

    line("\n== 10.1.4 Dynamic hedging: re-immunise every half year along a yield path ==")
    path = [(0.0, 0.06), (0.5, 0.065), (1.0, 0.07), (1.5, 0.06), (2.0, 0.05), (2.5, 0.055), (3.0, 0.065),
            (3.5, 0.07), (4.0, 0.075), (4.5, 0.07), (5.0, 0.065)]
    for st in dynamic_hedge(V0, A, B, 5.0, path):
        line(f"t={f(st.t)} y={f(st.y)}: value={f(st.value)} q_A={f(st.q1)} q_B={f(st.q2)} D={f(st.duration)}")
    static = dynamic_hedge(V0, A, B, 5.0, path, rebalance=False)
    line(f"At t=5: rebalanced {f(dynamic_hedge(V0, A, B, 5.0, path)[-1].value)}, "
         f"not rebalanced {f(static[-1].value)}, target {f(target)}")

    line("\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==")
    Ts = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
    Bs = [0.978, 0.953, 0.926, 0.898, 0.870, 0.842]
    fw = forward_rates(Ts, Bs)
    for T, Bv, fr in zip(Ts, Bs, fw):
        line(f"T={f(T)}: B={f(Bv)} y(0,T)={f(yield_rate(Bv, T))} f(0,T-0.5,T)={f(fr)}")
    line(f"Prices rebuilt from the forward rates: {fl(prices_from_forward_rates(Ts, fw))}")
    line(f"f(0,1,3)={f(forward_rate(Bs[1], 1.0, Bs[5], 3.0))} = average of the half-year forwards "
         f"{f(plain_sum(fw[2:6]) / 4)}")
    line("Implied B(1,T) = B(0,T)/B(0,1): " + " ".join(f"T={f(T)}:{f(implied_price(Bs[1], Bv))}" for T, Bv in zip(Ts[2:], Bs[2:])))
    line(f"FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)={f(forward_rate(Bs[1], 1.0, Bs[3], 2.0))}: "
         f"buy 1000 one-year bonds (cost {f(1000.0 * Bs[1])}), sell {f(1000.0 * Bs[1] / Bs[3])} two-year bonds; "
         f"repay {f(1000.0 * math.exp(forward_rate(Bs[1], 1.0, Bs[3], 2.0)))}")
    ft = [1.0, 2.0, 3.0]
    fC = [6.0, 6.0, 106.0]
    P = coupon_bond_price(ft, fC, Ts, Bs)
    line(f"3-year 6% coupon bond under the term structure: price {f(P)}, yield to maturity {f(yield_to_maturity(P, ft, fC))} "
         f"(between y(0,1)={f(yield_rate(Bs[1], 1.0))} and y(0,3)={f(yield_rate(Bs[5], 3.0))})")

    line("\n== 10.2.2 Money market account: rolling over half-year bonds at the forward rates ==")
    As = money_market_account(Ts, Bs)
    for T, a, Bv in zip(Ts, As, Bs):
        line(f"A({f(T)})={f(a)} = 1/B(0,T)={f(1 / Bv)}")
    line(f"Short rates realised = forward rates: {fl(fw)}; A(3)=exp(sum f dt)={f(math.exp(plain_sum(fr * 0.5 for fr in fw)))}")
    line(f"1 unit at t=0 in the 3-year bond: {f(1 / Bs[5])} at t=3, same as the money market account {f(As[5])}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
