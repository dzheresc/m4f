#!/usr/bin/env python3
"""Chapter 4 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
from no_arbitrage import (arbitrage_strategy, expected_discounted_value, is_arbitrage_free,
                          one_step_values, risk_neutral_probs)
from strategies import discounted_gains, gains, self_financing, self_financing_multi


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def contrarian(n: int, moves: list[int]) -> float:
    """Hold one share plus one more for every down move so far."""
    return 1.0 + (len(moves) - sum(moves))


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 4: Discrete Time Market Models (illustrative numbers, book formulas)")

    line("\n== 4.1.1 Investment strategies: self-financing x(n+1)S(n) + y(n+1)A(n) = V(n) ==")
    S = [100.0, 110.0, 104.5, 114.95]
    A = [100.0 * 1.03 ** n for n in range(4)]
    x = [5.0, 8.0, 3.0]
    y, V = self_financing(1000.0, S, A, x)
    line("V(0)=1000, A(n)=100*1.03^n")
    for n in range(1, 4):
        line(f"n={n}: S={f(S[n])} A={f(A[n])} x={f(x[n - 1])} y={f(y[n - 1])} V={f(V[n])}")
    line(f"V(3)-V(0)={f(V[3] - V[0])} gains sum={f(gains(S, A, x, y))}")
    line(f"V~(3)-V~(0)={f(V[3] / A[3] - V[0] / A[0])} discounted gains={f(discounted_gains(S, A, x))}")

    line("\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, K in {0.10, -0.05} ==")
    for r in (0.12, 0.03, -0.05):
        rets = [0.10, -0.05]
        if is_arbitrage_free(rets, r):
            line(f"r={f(r)}: no arbitrage")
        else:
            xs, ys = arbitrage_strategy(rets, r, 100.0, 100.0)
            line(f"r={f(r)}: arbitrage x={f(xs)} y={f(ys)} V(0)={f(xs * 100.0 + ys * 100.0)} "
                 f"V(1)={fl(one_step_values(xs, ys, rets, r, 100.0, 100.0))}")

    line("\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, x(n) = 1 + #downs so far ==")
    q = (0.03 - -0.05) / (0.10 - -0.05)
    line(f"V(0)=100: E*(V~(4))={f(expected_discounted_value(100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, q))} "
         f"(p*={f(q)}), E(V~(4))={f(expected_discounted_value(100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, 0.6))} (p=0.6)")

    line("\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, K in {-0.10, 0, 0.10} ==")
    rets = [-0.10, 0.0, 0.10]
    for r in (-0.15, 0.02, 0.05, 0.10):
        qs = risk_neutral_probs(rets, r)
        if qs is None:
            line(f"r={f(r)}: arbitrage, no risk-neutral probability")
        else:
            line(f"r={f(r)}: q={fl(qs)} sum qK={f(sum(a * b for a, b in zip(qs, rets)))}")

    line("\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, A(n)=1.05^n ==")
    S2 = [[50.0, 55.0, 52.0, 58.0], [20.0, 19.0, 21.0, 22.0]]
    A2 = [1.05 ** n for n in range(4)]
    x2 = [[10.0, 12.0, 8.0], [20.0, 15.0, 25.0]]
    div = [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 2.0, 0.0]]
    y2, V2 = self_financing_multi(1000.0, S2, A2, x2, div)
    for n in range(1, 4):
        line(f"n={n}: x1={f(x2[0][n - 1])} x2={f(x2[1][n - 1])} y={f(y2[n - 1])} V={f(V2[n])}")
    _, Vnd = self_financing_multi(1000.0, S2, A2, x2)
    line(f"Without the dividend V(3)={f(Vnd[3])}; it adds {f(V2[3] - Vnd[3])} = 2 x 15 x 1.05")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
