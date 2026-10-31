#!/usr/bin/env python3
"""Chapter 8 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import binomial_pricing
import black_scholes


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def call(strike: float) -> binomial_pricing.Payoff:
    """Payoff of a call with the given strike."""
    return lambda s: max(s - strike, 0.0)


def put(strike: float) -> binomial_pricing.Payoff:
    """Payoff of a put with the given strike."""
    return lambda s: max(strike - s, 0.0)


def bull_spread(s: float) -> float:
    """Bull spread 90/120."""
    return min(max(s - 90.0, 0.0), 30.0)


def one_step_section(out: list[str]) -> None:
    """Section 8.1.1."""
    out.append(
        "\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 =="
    )
    x, y, price = binomial_pricing.one_step_replication(
        100.0, 0.2, -0.1, 0.1, 20.0, 0.0
    )
    q = binomial_pricing.risk_neutral_prob(0.2, -0.1, 0.1)
    out.append(
        f"Replicate with x={fmt(x)} shares, y={fmt(y)} in the money market: "
        f"C(0)={fmt(price)}"
    )
    out.append(
        f"Risk-neutral: p*={fmt(q)}, C(0) = E*(C(1))/(1+r) = "
        f"{fmt((q * 20.0 + (1 - q) * 0.0) / 1.1)}"
    )


def two_steps_section(out: list[str]) -> None:
    """Section 8.1.2."""
    out.append("\n== 8.1.2 Two steps: same tree, call X=100 ==")
    values = binomial_pricing.option_tree(100.0, 0.2, -0.1, 0.1, 2, call(100.0))
    for n in range(3):
        s = [
            binomial_pricing.stock_price(100.0, 0.2, -0.1, n, k)
            for k in range(n, -1, -1)
        ]
        out.append(f"n={n}: S={fmt_list(s)} C={fmt_list(values[n][::-1])}")
    for n, k in ((0, 0), (1, 1), (1, 0)):
        x, y = binomial_pricing.replicating_position(
            100.0, 0.2, -0.1, 0.1, values, n, k
        )
        out.append(f"node ({n},{k}): x={fmt(x)} y={fmt(y)}")


def general_model_section(out: list[str]) -> None:
    """Sections 8.1.3 and 8.1.4."""
    out.append("\n== 8.1.3 General N-step model: N=5, X=100 ==")
    for name, payoff in (("call", call(100.0)), ("put", put(100.0))):
        tree = binomial_pricing.option_tree(100.0, 0.2, -0.1, 0.1, 5, payoff)
        total = binomial_pricing.european_price_sum(
            100.0, 0.2, -0.1, 0.1, 5, payoff
        )
        out.append(
            f"{name}: backward induction={fmt(tree[0][0])} "
            f"sum formula={fmt(total)}"
        )
    spread = binomial_pricing.european_price_sum(
        100.0, 0.2, -0.1, 0.1, 5, bull_spread
    )
    out.append(f"bull spread 90/120: {fmt(spread)}")

    out.append("\n== 8.1.4 Cox-Ross-Rubinstein formula ==")
    for strike in (80.0, 100.0, 150.0):
        price, m = binomial_pricing.crr_call(100.0, strike, 0.2, -0.1, 0.1, 5)
        total = binomial_pricing.european_price_sum(
            100.0, 0.2, -0.1, 0.1, 5, call(strike)
        )
        out.append(
            f"N=5 X={fmt(strike)}: m={m} C={fmt(price)} "
            f"sum formula={fmt(total)}"
        )


def american_section(out: list[str]) -> None:
    """Section 8.2."""
    out.append(
        "\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put "
        "X=100 =="
    )
    american = binomial_pricing.option_tree(
        100.0, 0.1, -0.1, 0.02, 4, put(100.0), american=True
    )
    european = binomial_pricing.option_tree(
        100.0, 0.1, -0.1, 0.02, 4, put(100.0)
    )
    out.append(f"P_A={fmt(american[0][0])} P_E={fmt(european[0][0])}")
    for n in range(5):
        out.append(f"n={n}: P_A={fmt_list(american[n][::-1])}")
    nodes = binomial_pricing.early_exercise_nodes(
        100.0, 0.1, -0.1, 0.02, 4, put(100.0)
    )
    out.append(
        "Exercise early at (n,k): "
        + ", ".join(
            f"({n},{k}) "
            f"S={fmt(binomial_pricing.stock_price(100.0, 0.1, -0.1, n, k))}"
            for n, k in nodes
        )
    )


def black_scholes_section(out: list[str]) -> None:
    """Section 8.3."""
    out.append(
        "\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 =="
    )
    out.append(
        f"C={fmt(black_scholes.bs_call(100.0, 100.0, 0.05, 0.2, 1.0))} "
        f"P={fmt(black_scholes.bs_put(100.0, 100.0, 0.05, 0.2, 1.0))} "
        "delta="
        f"{fmt(black_scholes.bs_call_delta(100.0, 100.0, 0.05, 0.2, 1.0))}"
    )
    for num_steps in (10, 50, 100, 500, 1000):
        prices = [
            black_scholes.crr_tree_price(
                100.0, 100.0, 0.05, 0.2, 1.0, num_steps, is_call, american
            )
            for is_call, american in (
                (True, False),
                (False, False),
                (False, True),
            )
        ]
        out.append(
            f"CRR N={num_steps}: C={fmt(prices[0])} P={fmt(prices[1])} "
            f"P_A={fmt(prices[2])}"
        )


def report() -> str:
    """The full chapter 8 report."""
    out = ["Chapter 8: Option Pricing (illustrative numbers, book formulas)"]
    one_step_section(out)
    two_steps_section(out)
    general_model_section(out)
    american_section(out)
    black_scholes_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
