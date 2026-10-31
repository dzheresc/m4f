#!/usr/bin/env python3
"""Chapter 7 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import math

import binomial
import options


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def price(
    s0: float,
    strike: float,
    maturity: float,
    is_call: bool,
    american: bool = False,
) -> float:
    """The model price used throughout: r = 0.05, sigma = 0.2, 200 steps."""
    return binomial.crr_price(
        s0, strike, 0.05, 0.2, maturity, 200, is_call, american
    )


def definitions_section(out: list[str]) -> None:
    """Section 7.1."""
    out.append("\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==")
    for s_t in (80.0, 100.0, 120.0):
        call = options.call_payoff(s_t, 100.0)
        put = options.put_payoff(s_t, 100.0)
        out.append(
            f"S(T)={fmt(s_t)}: call payoff={fmt(call)} "
            f"profit long={fmt(options.profit(call, 10.0, 0.05, 1.0))} "
            f"short={fmt(-options.profit(call, 10.0, 0.05, 1.0))}; "
            f"put payoff={fmt(put)} "
            f"profit long={fmt(options.profit(put, 5.0, 0.05, 1.0))} "
            f"short={fmt(-options.profit(put, 5.0, 0.05, 1.0))}"
        )


def parity_section(out: list[str]) -> None:
    """Section 7.2."""
    out.append(
        "\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 "
        "r=0.05 T=1 =="
    )
    out.append(f"S(0) - X e^(-rT) = {fmt(100.0 - 100.0 * math.exp(-0.05))}")
    for call, put in ((10.45, 5.57), (12.0, 5.0), (9.0, 6.0)):
        gap = options.parity_gap(call, put, 100.0, 100.0, 0.05, 1.0)
        out.append(
            f"C={fmt(call)} P={fmt(put)}: gap={fmt(gap)} "
            f"arbitrage profit at T={fmt(abs(gap) * math.exp(0.05))}"
        )
    pv_div = 2 * math.exp(-0.025)
    expected = -options.parity_gap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, pv_div)
    out.append(
        f"Dividend 2 at t=0.5 (PV {fmt(pv_div)}): C - P should be "
        f"{fmt(expected)}"
    )


def european_bounds_section(out: list[str]) -> None:
    """Section 7.3.1."""
    out.append(
        "\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 =="
    )
    call_bounds = options.european_call_bounds(100.0, 100.0, 0.05, 1.0)
    put_bounds = options.european_put_bounds(100.0, 100.0, 0.05, 1.0)
    out.append(
        f"call in {fmt_list(list(call_bounds))}, "
        f"put in {fmt_list(list(put_bounds))}"
    )
    model_call = price(100.0, 100.0, 1.0, True)
    out.append(
        "Quoted call 3.50: violation "
        f"{fmt(options.bound_violation(3.5, call_bounds))}; "
        "quoted put 96.00: violation "
        f"{fmt(options.bound_violation(96.0, put_bounds))}; "
        f"model call {fmt(model_call)}: violation "
        f"{fmt(options.bound_violation(model_call, call_bounds))}"
    )


def american_section(out: list[str]) -> None:
    """Sections 7.3.2 and 7.3.3."""
    out.append(
        "\n== 7.3.2 American calls on a non-dividend paying stock (CRR, "
        "sigma=0.2, N=200) =="
    )
    for s0 in (90.0, 100.0, 120.0):
        call_e = price(s0, 100.0, 1.0, True)
        call_a = price(s0, 100.0, 1.0, True, True)
        out.append(
            f"S(0)={fmt(s0)}: C_E={fmt(call_e)} C_A={fmt(call_a)} "
            f"exercise now={fmt(options.call_payoff(s0, 100.0))}"
        )

    out.append(
        "\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X "
        "e^(-rT) =="
    )
    for s0 in (90.0, 100.0, 120.0):
        put_e = price(s0, 100.0, 1.0, False)
        put_a = price(s0, 100.0, 1.0, False, True)
        call_a = price(s0, 100.0, 1.0, True, True)
        lo, hi = options.american_parity_bounds(s0, 100.0, 0.05, 1.0)
        out.append(
            f"S(0)={fmt(s0)}: P_E={fmt(put_e)} P_A={fmt(put_a)} "
            f"C_A-P_A={fmt(call_a - put_a)} in {fmt_list([lo, hi])}"
        )


def variables_section(out: list[str]) -> None:
    """Sections 7.4.1 and 7.4.2."""
    out.append("\n== 7.4.1 European options: dependence on X and S(0) ==")
    strikes = [90.0, 100.0, 110.0]
    calls = [price(100.0, x, 1.0, True) for x in strikes]
    puts = [price(100.0, x, 1.0, False) for x in strikes]
    out.append(f"X={fmt_list(strikes)}: C={fmt_list(calls)} P={fmt_list(puts)}")
    gap = options.butterfly_gap(
        calls[0], calls[1], calls[2], 90.0, 100.0, 110.0
    )
    out.append(
        f"C(90)-C(100)={fmt(calls[0] - calls[1])} <= e^(-rT) x 10 = "
        f"{fmt(10 * math.exp(-0.05))}; butterfly gap={fmt(gap)} (<= 0)"
    )
    quoted_gap = options.butterfly_gap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0)
    out.append(
        f"Quotes C=15, 10, 2: butterfly gap={fmt(quoted_gap)} "
        "-> write 2 x C(100), buy C(90) and C(110)"
    )
    spots = [90.0, 100.0, 110.0]
    calls_by_spot = [price(s, 100.0, 1.0, True) for s in spots]
    out.append(f"S(0)={fmt_list(spots)}: C={fmt_list(calls_by_spot)}")

    out.append("\n== 7.4.2 American options: dependence on T ==")
    maturities = [0.25, 0.5, 1.0, 2.0]
    american_puts = [price(100.0, 100.0, t, False, True) for t in maturities]
    out.append(f"T={fmt_list(maturities)}: P_A={fmt_list(american_puts)}")


def time_value_section(out: list[str]) -> None:
    """Section 7.5."""
    out.append(
        "\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 =="
    )
    for s0 in (80.0, 100.0, 120.0):
        call = price(s0, 100.0, 1.0, True)
        put = price(s0, 100.0, 1.0, False)
        out.append(
            f"S(0)={fmt(s0)}: call {fmt(call)} "
            f"time value {fmt(call - options.call_payoff(s0, 100.0))}; "
            f"put {fmt(put)} "
            f"time value {fmt(put - options.put_payoff(s0, 100.0))}"
        )


def report() -> str:
    """The full chapter 7 report."""
    out = [
        "Chapter 7: Options: General Properties (illustrative numbers, book"
        " formulas)"
    ]
    definitions_section(out)
    parity_section(out)
    european_bounds_section(out)
    american_section(out)
    variables_section(out)
    time_value_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
