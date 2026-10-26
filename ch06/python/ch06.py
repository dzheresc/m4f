#!/usr/bin/env python3
"""Chapter 6 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from forwards import (forward_arbitrage_profit, forward_price, forward_price_dividends,
                      forward_price_yield, long_forward_value)
from futures import futures_tree, hedged_std, margin_account, min_variance_hedge_ratio


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 6: Forward and Futures Contracts (illustrative numbers, book formulas)")

    line("\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 T=0.75 ==")
    F0 = forward_price(100.0, 0.08, 0.75)
    line(f"No dividends: F(0,T)={f(F0)}")
    line(f"Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)="
         f"{f(forward_price_dividends(100.0, 0.08, 0.75, [(2.0, 0.25), (2.0, 0.5)]))}")
    line(f"Dividend yield 3%: F(0,T)={f(forward_price_yield(100.0, 0.08, 0.03, 0.75))}")
    line(f"Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: F={f(forward_price_yield(1.25, 0.05, 0.03, 1.0))}")
    for Fq in (108.0, 104.0):
        side = "cash and carry: buy stock, sell forward" if Fq > F0 else "reverse: short stock, buy forward"
        line(f"Quoted F={f(Fq)}: {side}, profit at T={f(forward_arbitrage_profit(Fq, 100.0, 0.08, 0.75))}")

    line("\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) e^(-r(T-t)) ==")
    for St in (104.0, 98.0):
        Ft = forward_price(St, 0.08, 0.25)
        line(f"t=0.5 S(t)={f(St)}: F(t,T)={f(Ft)} long V(t)={f(long_forward_value(F0, Ft, 0.08, 0.25))} "
             f"short V(t)={f(-long_forward_value(F0, Ft, 0.08, 0.25))}")

    line("\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 d=-0.05 r=0.03 N=3 ==")
    tree = futures_tree(100.0, 0.10, -0.05, 0.03, 3)
    for n in range(4):
        fwd = [100.0 * 1.1 ** k * 0.95 ** (n - k) * 1.03 ** (3 - n) for k in range(n, -1, -1)]
        line(f"n={n}: futures={fl(tree[n][::-1])} forward={fl(fwd)}")
    line("Marking to market: 100 units, initial margin 1000, maintenance 750")
    prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0]
    for n, (bal, call) in enumerate(margin_account(prices, 100.0, 1000.0, 750.0), start=1):
        line(f"day {n}: f={f(prices[n])} balance={f(bal)} margin call={f(call)}")

    line("\n== 6.2.2 Hedging with futures: four equally likely scenarios ==")
    dS, df, p = [-6.0, -1.0, 2.0, 5.0], [-5.0, -2.0, 3.0, 4.0], [0.25, 0.25, 0.25, 0.25]
    h = min_variance_hedge_ratio(dS, df, p)
    line(f"dS={fl(dS)} df={fl(df)}")
    line(f"h*={f(h)}; std of spot change: unhedged {f(hedged_std(0.0, dS, df, p))}, "
         f"h=1 {f(hedged_std(1.0, dS, df, p))}, h* {f(hedged_std(h, dS, df, p))}")
    line(f"Perfect hedge: buy at T with a long futures at f(0)=106; spot at T 112 or 95 -> "
         f"cost {fl([112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)])}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
