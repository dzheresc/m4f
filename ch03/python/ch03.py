#!/usr/bin/env python3
"""Chapter 3 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
import math

from binomial_tree import BinomialTree, binomial_prob
from other_models import (TrinomialModel, binomial_expected_price, binomial_prob_above,
                          lognormal_expected_price, lognormal_prob_above)
from returns import (compound_return, expectation, log_return, log_returns, one_step_return,
                     returns, variance)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 3: Risky Assets (illustrative numbers, book formulas)")

    line("\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), k(n) = ln(S(n) / S(n-1)) ==")
    path = [45.0, 48.0, 47.5, 50.0]
    Ks, ks = returns(path), log_returns(path)
    for n in range(1, len(path)):
        line(f"n={n}: S={f(path[n])} K={f(Ks[n - 1])} k={f(ks[n - 1])}")
    line(f"K(0,3) from S: {f(one_step_return(path[0], path[3]))}, "
         f"from (1+K(1))(1+K(2))(1+K(3))-1: {f(compound_return(Ks))}")
    line(f"k(0,3) from S: {f(log_return(path[0], path[3]))}, "
         f"k(1)+k(2)+k(3): {f(ks[0] + ks[1] + ks[2])}")
    line(f"S(0)=45, S(1)=48 with dividend 2: K(1)={f(one_step_return(45.0, 48.0, 2.0))}")

    line("\n== 3.1.2 Expected return ==")
    S1s, probs = [52.0, 48.0, 40.0], [0.25, 0.5, 0.25]
    K1s = [one_step_return(45.0, s) for s in S1s]
    EK = expectation(K1s, probs)
    line(f"S(0)=45, S(1) in {{52, 48, 40}} with p={{0.25, 0.5, 0.25}}: "
         f"E(K)={f(EK)} sd(K)={f(math.sqrt(variance(K1s, probs)))}")
    line(f"E(S(1)) = (1 + E(K)) S(0) = {f((1 + EK) * 45.0)}, directly {f(expectation(S1s, probs))}")
    line(f"Independent steps, E(K(1))=0.10, E(K(2))=0.05: "
         f"E(K(0,2)) = {f(compound_return([0.10, 0.05]))}")

    line("\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 ==")
    tree = BinomialTree(100.0, 0.10, -0.05, 0.03)
    for n in range(4):
        nodes = " ".join(f"{f(tree.price(n, k))}({f(binomial_prob(n, k, 0.6))})"
                         for k in range(n, -1, -1))
        line(f"n={n}: {nodes}")
    line(f"E(K(n))={f(tree.expected_return(0.6))} E(S(3))={f(tree.expected_price(3, 0.6))} "
         f"(1+E(K))^3 S(0)={f((1 + tree.expected_return(0.6)) ** 3 * 100.0)}")

    line("\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==")
    q = tree.risk_neutral_prob()
    line(f"d < r < u: {'yes' if tree.is_arbitrage_free() else 'no'}, p*={f(q)}, "
         f"E*(K)={f(tree.expected_return(q))}")
    line(f"E*(S(1))={f(tree.expected_price(1, q))} (1+r)S(0)={f(1.03 * 100.0)}; "
         f"E*(S(3))={f(tree.expected_price(3, q))} (1+r)^3 S(0)={f(1.03 ** 3 * 100.0)}")

    line("\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==")
    worst = 0.0
    for n in range(3):
        for k in range(n + 1):
            worst = max(worst, abs(tree.conditional_discounted(n, k) - tree.discounted_price(n, k)))
    for k in range(1, -1, -1):
        line(f"node (1,{k}): S~(1)={f(tree.discounted_price(1, k))} "
             f"E*(S~(2)|.)={f(tree.conditional_discounted(1, k))}")
    line(f"Largest deviation over all nodes n<3: {f(worst)}")

    line("\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 ==")
    tri = TrinomialModel(0.10, 0.0, -0.10, 0.02)
    line(f"Risk-neutral probabilities exist for 0 < p_m < {f(tri.max_middle_prob())}")
    for pm in (0.2, 0.4, 0.6):
        pu, _, pd = tri.risk_neutral_probs(pm)
        call = pu * 10.0 / 1.02
        line(f"p_m={f(pm)}: p_u={f(pu)} p_d={f(pd)} "
             f"E*(K)={f(pu * 0.10 + pm * 0.0 + pd * -0.10)} call(K=100) price={f(call)}")

    line("\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 ==")
    for N in (1, 10, 100, 1000, 10000):
        line(f"N={N}: E(S(T))={f(binomial_expected_price(100.0, 0.05, 0.2, 1.0, N))} "
             f"P(S(T)>110)={f(binomial_prob_above(100.0, 0.05, 0.2, 1.0, N, 110.0))}")
    line(f"Limit: E(S(T))={f(lognormal_expected_price(100.0, 0.05, 0.2, 1.0))} "
         f"P(S(T)>110)={f(lognormal_prob_above(100.0, 0.05, 0.2, 1.0, 110.0))}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
