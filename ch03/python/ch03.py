#!/usr/bin/env python3
"""Chapter 3 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import math

import binomial_tree
import other_models
import returns


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def returns_section(out: list[str]) -> None:
    """Section 3.1.1."""
    out.append(
        "\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), "
        "k(n) = ln(S(n) / S(n-1)) =="
    )
    path = [45.0, 48.0, 47.5, 50.0]
    one_step = returns.returns(path)
    logs = returns.log_returns(path)
    for n in range(1, len(path)):
        out.append(
            f"n={n}: S={fmt(path[n])} K={fmt(one_step[n - 1])} "
            f"k={fmt(logs[n - 1])}"
        )
    out.append(
        f"K(0,3) from S: {fmt(returns.one_step_return(path[0], path[3]))}, "
        "from (1+K(1))(1+K(2))(1+K(3))-1: "
        f"{fmt(returns.compound_return(one_step))}"
    )
    out.append(
        f"k(0,3) from S: {fmt(returns.log_return(path[0], path[3]))}, "
        f"k(1)+k(2)+k(3): {fmt(logs[0] + logs[1] + logs[2])}"
    )
    out.append(
        "S(0)=45, S(1)=48 with dividend 2: "
        f"K(1)={fmt(returns.one_step_return(45.0, 48.0, 2.0))}"
    )


def expected_return_section(out: list[str]) -> None:
    """Section 3.1.2."""
    out.append("\n== 3.1.2 Expected return ==")
    prices, probs = [52.0, 48.0, 40.0], [0.25, 0.5, 0.25]
    one_step = [returns.one_step_return(45.0, s1) for s1 in prices]
    mean = returns.expectation(one_step, probs)
    sd = math.sqrt(returns.variance(one_step, probs))
    out.append(
        "S(0)=45, S(1) in {52, 48, 40} with p={0.25, 0.5, 0.25}: "
        f"E(K)={fmt(mean)} sd(K)={fmt(sd)}"
    )
    out.append(
        f"E(S(1)) = (1 + E(K)) S(0) = {fmt((1 + mean) * 45.0)}, "
        f"directly {fmt(returns.expectation(prices, probs))}"
    )
    out.append(
        "Independent steps, E(K(1))=0.10, E(K(2))=0.05: "
        f"E(K(0,2)) = {fmt(returns.compound_return([0.10, 0.05]))}"
    )


def binomial_section(out: list[str], tree: binomial_tree.BinomialTree) -> None:
    """Sections 3.2, 3.2.1 and 3.2.2."""
    out.append(
        "\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 =="
    )
    for n in range(4):
        nodes = " ".join(
            f"{fmt(tree.price(n, k))}"
            f"({fmt(binomial_tree.binomial_prob(n, k, 0.6))})"
            for k in range(n, -1, -1)
        )
        out.append(f"n={n}: {nodes}")
    out.append(
        f"E(K(n))={fmt(tree.expected_return(0.6))} "
        f"E(S(3))={fmt(tree.expected_price(3, 0.6))} "
        f"(1+E(K))^3 S(0)={fmt((1 + tree.expected_return(0.6)) ** 3 * 100.0)}"
    )

    out.append("\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==")
    q = tree.risk_neutral_prob()
    free = "yes" if tree.is_arbitrage_free() else "no"
    out.append(
        f"d < r < u: {free}, p*={fmt(q)}, E*(K)={fmt(tree.expected_return(q))}"
    )
    out.append(
        f"E*(S(1))={fmt(tree.expected_price(1, q))} "
        f"(1+r)S(0)={fmt(1.03 * 100.0)}; "
        f"E*(S(3))={fmt(tree.expected_price(3, q))} "
        f"(1+r)^3 S(0)={fmt(1.03 ** 3 * 100.0)}"
    )

    out.append("\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==")
    worst = 0.0
    for n in range(3):
        for k in range(n + 1):
            worst = max(
                worst,
                abs(
                    tree.conditional_discounted(n, k)
                    - tree.discounted_price(n, k)
                ),
            )
    for k in range(1, -1, -1):
        out.append(
            f"node (1,{k}): S~(1)={fmt(tree.discounted_price(1, k))} "
            f"E*(S~(2)|.)={fmt(tree.conditional_discounted(1, k))}"
        )
    out.append(f"Largest deviation over all nodes n<3: {fmt(worst)}")


def trinomial_section(out: list[str]) -> None:
    """Section 3.3.1."""
    out.append(
        "\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 =="
    )
    model = other_models.TrinomialModel(0.10, 0.0, -0.10, 0.02)
    out.append(
        "Risk-neutral probabilities exist for 0 < p_m < "
        f"{fmt(model.max_middle_prob())}"
    )
    for p_mid in (0.2, 0.4, 0.6):
        p_up, _, p_down = model.risk_neutral_probs(p_mid)
        call = p_up * 10.0 / 1.02
        expected = p_up * 0.10 + p_mid * 0.0 + p_down * -0.10
        out.append(
            f"p_m={fmt(p_mid)}: p_u={fmt(p_up)} p_d={fmt(p_down)} "
            f"E*(K)={fmt(expected)} call(K=100) price={fmt(call)}"
        )


def continuous_limit_section(out: list[str]) -> None:
    """Section 3.3.2."""
    out.append(
        "\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 =="
    )
    for num_steps in (1, 10, 100, 1000, 10000):
        mean = other_models.binomial_expected_price(
            100.0, 0.05, 0.2, 1.0, num_steps
        )
        prob = other_models.binomial_prob_above(
            100.0, 0.05, 0.2, 1.0, num_steps, 110.0
        )
        out.append(
            f"N={num_steps}: E(S(T))={fmt(mean)} P(S(T)>110)={fmt(prob)}"
        )
    mean = other_models.lognormal_expected_price(100.0, 0.05, 0.2, 1.0)
    prob = other_models.lognormal_prob_above(100.0, 0.05, 0.2, 1.0, 110.0)
    out.append(f"Limit: E(S(T))={fmt(mean)} P(S(T)>110)={fmt(prob)}")


def report() -> str:
    """Returns the chapter's worked examples as text."""
    out = ["Chapter 3: Risky Assets (illustrative numbers, book formulas)"]
    returns_section(out)
    expected_return_section(out)
    binomial_section(out, binomial_tree.BinomialTree(100.0, 0.10, -0.05, 0.03))
    trinomial_section(out)
    continuous_limit_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
