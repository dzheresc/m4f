#!/usr/bin/env python3
"""Chapter 4 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import no_arbitrage
import strategies


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def contrarian(n: int, moves: list[int]) -> float:  # pylint: disable=unused-argument
    """Hold one share plus one more for every down move so far."""
    return 1.0 + (len(moves) - sum(moves))


def strategies_section(out: list[str]) -> None:
    """Section 4.1.1."""
    out.append(
        "\n== 4.1.1 Investment strategies: self-financing "
        "x(n+1)S(n) + y(n+1)A(n) = V(n) =="
    )
    s = [100.0, 110.0, 104.5, 114.95]
    a = [100.0 * 1.03**n for n in range(4)]
    x = [5.0, 8.0, 3.0]
    y, values = strategies.self_financing(1000.0, s, a, x)
    out.append("V(0)=1000, A(n)=100*1.03^n")
    for n in range(1, 4):
        out.append(
            f"n={n}: S={fmt(s[n])} A={fmt(a[n])} x={fmt(x[n - 1])} "
            f"y={fmt(y[n - 1])} V={fmt(values[n])}"
        )
    out.append(
        f"V(3)-V(0)={fmt(values[3] - values[0])} "
        f"gains sum={fmt(strategies.gains(s, a, x, y))}"
    )
    out.append(
        f"V~(3)-V~(0)={fmt(values[3] / a[3] - values[0] / a[0])} "
        f"discounted gains={fmt(strategies.discounted_gains(s, a, x))}"
    )


def no_arbitrage_section(out: list[str]) -> None:
    """Section 4.1.2."""
    out.append(
        "\n== 4.1.2 Principle of no arbitrage: S(0)=100 A(0)=100, "
        "K in {0.10, -0.05} =="
    )
    for r in (0.12, 0.03, -0.05):
        returns = [0.10, -0.05]
        if no_arbitrage.is_arbitrage_free(returns, r):
            out.append(f"r={fmt(r)}: no arbitrage")
        else:
            x, y = no_arbitrage.arbitrage_strategy(returns, r, 100.0, 100.0)
            values = no_arbitrage.one_step_values(
                x, y, returns, r, 100.0, 100.0
            )
            out.append(
                f"r={fmt(r)}: arbitrage x={fmt(x)} y={fmt(y)} "
                f"V(0)={fmt(x * 100.0 + y * 100.0)} V(1)={fmt_list(values)}"
            )


def binomial_section(out: list[str]) -> None:
    """Section 4.1.3."""
    out.append(
        "\n== 4.1.3 Binomial tree: S(0)=100 u=0.10 d=-0.05 r=0.03 N=4, "
        "x(n) = 1 + #downs so far =="
    )
    q = (0.03 - -0.05) / (0.10 - -0.05)
    risk_neutral = no_arbitrage.expected_discounted_value(
        100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, q
    )
    real = no_arbitrage.expected_discounted_value(
        100.0, 0.10, -0.05, 0.03, 4, 100.0, contrarian, 0.6
    )
    out.append(
        f"V(0)=100: E*(V~(4))={fmt(risk_neutral)} (p*={fmt(q)}), "
        f"E(V~(4))={fmt(real)} (p=0.6)"
    )


def fundamental_theorem_section(out: list[str]) -> None:
    """Section 4.1.4."""
    out.append(
        "\n== 4.1.4 Fundamental Theorem: no arbitrage <=> risk-neutral q > 0, "
        "K in {-0.10, 0, 0.10} =="
    )
    returns = [-0.10, 0.0, 0.10]
    for r in (-0.15, 0.02, 0.05, 0.10):
        q = no_arbitrage.risk_neutral_probs(returns, r)
        if q is None:
            out.append(f"r={fmt(r)}: arbitrage, no risk-neutral probability")
        else:
            mean = sum(a * b for a, b in zip(q, returns))
            out.append(f"r={fmt(r)}: q={fmt_list(q)} sum qK={fmt(mean)}")


def extended_models_section(out: list[str]) -> None:
    """Section 4.2."""
    out.append(
        "\n== 4.2 Extended models: two stocks, dividend 2 on stock 2 at n=2, "
        "A(n)=1.05^n =="
    )
    s = [[50.0, 55.0, 52.0, 58.0], [20.0, 19.0, 21.0, 22.0]]
    a = [1.05**n for n in range(4)]
    x = [[10.0, 12.0, 8.0], [20.0, 15.0, 25.0]]
    dividends = [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 2.0, 0.0]]
    y, values = strategies.self_financing_multi(1000.0, s, a, x, dividends)
    for n in range(1, 4):
        out.append(
            f"n={n}: x1={fmt(x[0][n - 1])} x2={fmt(x[1][n - 1])} "
            f"y={fmt(y[n - 1])} V={fmt(values[n])}"
        )
    _, without = strategies.self_financing_multi(1000.0, s, a, x)
    out.append(
        f"Without the dividend V(3)={fmt(without[3])}; "
        f"it adds {fmt(values[3] - without[3])} = 2 x 15 x 1.05"
    )


def report() -> str:
    """Returns the chapter's worked examples as text."""
    out = [
        "Chapter 4: Discrete Time Market Models "
        "(illustrative numbers, book formulas)"
    ]
    strategies_section(out)
    no_arbitrage_section(out)
    binomial_section(out)
    fundamental_theorem_section(out)
    extended_models_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
