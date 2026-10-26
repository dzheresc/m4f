#!/usr/bin/env python3
"""Chapter 8 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
from black_scholes import bs_call, bs_call_delta, bs_put, crr_tree_price
from binomial_pricing import (crr_call, early_exercise_nodes, european_price_sum, one_step_replication,
                              option_tree, replicating_position, risk_neutral_prob, stock_price)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def call(X: float):
    return lambda s: max(s - X, 0.0)


def put(X: float):
    return lambda s: max(X - s, 0.0)


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 8: Option Pricing (illustrative numbers, book formulas)")

    line("\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 ==")
    x, y, C = one_step_replication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0)
    q = risk_neutral_prob(0.2, -0.1, 0.1)
    line(f"Replicate with x={f(x)} shares, y={f(y)} in the money market: C(0)={f(C)}")
    line(f"Risk-neutral: p*={f(q)}, C(0) = E*(C(1))/(1+r) = {f((q * 20.0 + (1 - q) * 0.0) / 1.1)}")

    line("\n== 8.1.2 Two steps: same tree, call X=100 ==")
    V = option_tree(100.0, 0.2, -0.1, 0.1, 2, call(100.0))
    for n in range(3):
        line(f"n={n}: S={fl([stock_price(100.0, 0.2, -0.1, n, k) for k in range(n, -1, -1)])} "
             f"C={fl(V[n][::-1])}")
    for n, k in ((0, 0), (1, 1), (1, 0)):
        xs, ys = replicating_position(100.0, 0.2, -0.1, 0.1, V, n, k)
        line(f"node ({n},{k}): x={f(xs)} y={f(ys)}")

    line("\n== 8.1.3 General N-step model: N=5, X=100 ==")
    for name, pay in (("call", call(100.0)), ("put", put(100.0))):
        line(f"{name}: backward induction={f(option_tree(100.0, 0.2, -0.1, 0.1, 5, pay)[0][0])} "
             f"sum formula={f(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, pay))}")
    spread = lambda s: min(max(s - 90.0, 0.0), 30.0)  # noqa: E731 bull spread 90/120
    line(f"bull spread 90/120: {f(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, spread))}")

    line("\n== 8.1.4 Cox-Ross-Rubinstein formula ==")
    for X in (80.0, 100.0, 150.0):
        C, m = crr_call(100.0, X, 0.2, -0.1, 0.1, 5)
        line(f"N=5 X={f(X)}: m={m} C={f(C)} sum formula={f(european_price_sum(100.0, 0.2, -0.1, 0.1, 5, call(X)))}")

    line("\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put X=100 ==")
    VA = option_tree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), american=True)
    VE = option_tree(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
    line(f"P_A={f(VA[0][0])} P_E={f(VE[0][0])}")
    for n in range(5):
        line(f"n={n}: P_A={fl(VA[n][::-1])}")
    nodes = early_exercise_nodes(100.0, 0.1, -0.1, 0.02, 4, put(100.0))
    line("Exercise early at (n,k): " + ", ".join(f"({n},{k}) S={f(stock_price(100.0, 0.1, -0.1, n, k))}"
                                                  for n, k in nodes))

    line("\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 ==")
    line(f"C={f(bs_call(100.0, 100.0, 0.05, 0.2, 1.0))} P={f(bs_put(100.0, 100.0, 0.05, 0.2, 1.0))} "
         f"delta={f(bs_call_delta(100.0, 100.0, 0.05, 0.2, 1.0))}")
    for N in (10, 50, 100, 500, 1000):
        line(f"CRR N={N}: C={f(crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, N, True))} "
             f"P={f(crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, N, False))} "
             f"P_A={f(crr_tree_price(100.0, 100.0, 0.05, 0.2, 1.0, N, False, True))}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
