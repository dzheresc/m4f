#!/usr/bin/env python3
"""Chapter 1 worked examples.

The output is byte-identical to the C++ and Kotlin versions.
"""

import forward
import fx_arbitrage
import one_step

UP = one_step.State.UP
DOWN = one_step.State.DOWN


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def portfolio_line(
    market: one_step.OneStepMarket, name: str, portfolio: one_step.Portfolio
) -> str:
    k_up = market.portfolio_return(portfolio, UP)
    k_down = market.portfolio_return(portfolio, DOWN)
    return (
        f"{name}: x={fmt(portfolio.x)} y={fmt(portfolio.y)} "
        f"V(0)={fmt(market.value0(portfolio))} "
        f"V(1)={fmt(market.value1(portfolio, UP))}|"
        f"{fmt(market.value1(portfolio, DOWN))} "
        f"K_V={fmt(k_up)}|{fmt(k_down)}"
    )


def risk_line(
    market: one_step.OneStepMarket, name: str, portfolio: one_step.Portfolio
) -> str:
    k_up = market.portfolio_return(portfolio, UP)
    k_down = market.portfolio_return(portfolio, DOWN)
    return (
        f"{name}: x={fmt(portfolio.x)} y={fmt(portfolio.y)} "
        f"V(1)={fmt(market.value1(portfolio, UP))}|"
        f"{fmt(market.value1(portfolio, DOWN))} "
        f"K_V={fmt(k_up)}|{fmt(k_down)} "
        f"E(K_V)={fmt(market.expect(k_up, k_down))} "
        f"sigma_V={fmt(market.stddev(k_up, k_down))}"
    )


def option_line(
    market: one_step.OneStepMarket, name: str, d_up: float, d_down: float
) -> str:
    replica = market.replicate(d_up, d_down)
    return (
        f"{name}: payoff {fmt(d_up)}|{fmt(d_down)} "
        f"replicate x={fmt(replica.x)} y={fmt(replica.y)} "
        f"price={fmt(market.price(d_up, d_down))}"
    )


def arbitrage_lines(market: one_step.OneStepMarket) -> list[str]:
    free = "yes" if market.is_arbitrage_free else "no"
    lines = [
        f"d < r < u: {fmt(market.d)} < {fmt(market.r)} < {fmt(market.u)} "
        f"-> arbitrage-free: {free}"
    ]
    arb = market.arbitrage()
    if arb is not None:
        lines.append(
            f"Arbitrage: x={fmt(arb.x)} y={fmt(arb.y)} "
            f"V(0)={fmt(market.value0(arb))} "
            f"V(1)={fmt(market.value1(arb, UP))}|"
            f"{fmt(market.value1(arb, DOWN))}"
        )
    return lines


def returns_and_portfolios(out: list[str]) -> None:
    """Examples 1.1-1.2, Exercises 1.1-1.2 and the no-arbitrage check."""
    # The book leaves p unspecified (0 < p < 1); it does not affect anything
    # printed here.
    ex = one_step.OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)

    out.append("== Example 1.1: returns ==")
    out.append(
        f"A(0)={fmt(ex.a0)} A(1)={fmt(ex.a1)} S(0)={fmt(ex.s0)} "
        f"S(1)={fmt(ex.s_up)}|{fmt(ex.s_down)}"
    )
    out.append(f"K_A={fmt(ex.r)} K_S={fmt(ex.u)}|{fmt(ex.d)}")

    out.append("\n== Example 1.2: portfolio value and return ==")
    out.append(portfolio_line(ex, "Portfolio", one_step.Portfolio(20.0, 10.0)))

    # Exercises 1.1-1.2 (again p is left unspecified).
    exr = one_step.OneStepMarket(90.0, 100.0, 25.0, 30.0, 20.0, 0.5)
    out.append("\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 ==")
    out.append(portfolio_line(exr, "Portfolio", one_step.Portfolio(10.0, 15.0)))

    out.append("\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==")
    out.append(portfolio_line(exr, "Solution", exr.replicate(1160.0, 1040.0)))

    out.append("\n== No-arbitrage check on the Example 1.1 market ==")
    out.extend(arbitrage_lines(ex))


def currency_arbitrage(out: list[str]) -> None:
    """Exercise 1.3."""
    out.append(
        "\n== Exercise 1.3: currency dealers A (New York) and B (London) =="
    )
    fx = fx_arbitrage.FxMarket(
        [
            fx_arbitrage.FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
            fx_arbitrage.FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
            fx_arbitrage.FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
            fx_arbitrage.FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
        ]
    )
    cycle = fx.best_cycle("USD")
    route = " -> ".join(["USD"] + [leg.to for leg in cycle.legs])
    out.append(f"Best round trip: {route}")
    for leg in cycle.legs:
        out.append(
            f"  {leg.from_} -> {leg.to} with dealer {leg.dealer}: "
            f"rate {leg.rate:.6f}"
        )
    verdict = (
        "arbitrage (borrow USD, run the cycle, repay)"
        if cycle.growth > 1
        else "no arbitrage"
    )
    out.append(f"1 USD -> {cycle.growth:.6f} USD: {verdict}")


def binomial_tree(out: list[str]) -> None:
    """Example 1.4 / Figure 1.1 and the proof of Proposition 1.1."""
    # p is left unspecified by the book.
    market = one_step.OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
    s_up, s_down = market.s1(UP), market.s1(DOWN)
    out.append("\n== Example 1.4: one-step binomial tree (Figure 1.1) ==")
    out.append(
        f"A(0)={fmt(market.a0)} A(1)={fmt(market.a1)} S(0)={fmt(market.s0)} "
        f"S(1)={fmt(s_up)}|{fmt(s_down)}"
    )
    out.append(f"K_A={fmt(market.r)} K_S={fmt(market.u)}|{fmt(market.d)}")
    holds = "holds" if s_down < market.a1 < s_up else "fails"
    out.append(
        "Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: "
        f"{fmt(s_down)} < {fmt(market.a1)} < {fmt(s_up)} -> {holds}"
    )
    # Proof of Proposition 1.1: the two violating cases (illustrative S(1)
    # values).
    for up, down, label in (
        (130.0, 110.0, "A(1) <= S^d"),
        (110.0, 90.0, "A(1) >= S^u"),
    ):
        bad = one_step.OneStepMarket(100.0, 110.0, 100.0, up, down, 0.5)
        arb = bad.arbitrage()
        if arb is None:
            raise RuntimeError(f"expected an arbitrage in case {label}")
        out.append(
            f"  proof, case {label} (S(1)={fmt(up)}|{fmt(down)}): "
            f"x={fmt(arb.x)} y={fmt(arb.y)} V(0)={fmt(bad.value0(arb))} "
            f"V(1)={fmt(bad.value1(arb, UP))}|{fmt(bad.value1(arb, DOWN))}"
        )
    out.extend(arbitrage_lines(market))


def risk_and_return(out: list[str]) -> None:
    """Section 1.4 and Exercise 1.4: risk and return, $10,000 to invest."""
    market = one_step.OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)
    out.append(
        "\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, "
        "budget 10000 =="
    )
    out.append(risk_line(market, "Mixed", one_step.Portfolio(50.0, 60.0)))
    out.append(risk_line(market, "Bonds only", one_step.Portfolio(0.0, 100.0)))
    out.append(risk_line(market, "Stock only", one_step.Portfolio(125.0, 0.0)))
    out.append(
        risk_line(market, "Exercise 1.4 (50/50)", market.split(10000.0, 0.5))
    )


def forwards(out: list[str]) -> None:
    """Section 1.5: Example 1.5, Proposition 1.2, Exercises 1.5-1.6."""
    out.append("\n== 1.5 Forward contracts ==")
    out.append(f"Example 1.5: F={fmt(80.0)}")
    for s1 in (84.0, 75.0):
        out.append(
            f"  S(1)={fmt(s1)}: "
            f"long payoff {fmt(forward.long_forward_payoff(s1, 80.0))}, "
            f"short payoff {fmt(forward.short_forward_payoff(s1, 80.0))}"
        )
    out.append(
        "Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = "
        f"{fmt(forward.forward_price(50.0, 100.0, 110.0))}"
    )

    s0, a0, a1, quoted = 34.0, 100.0, 112.0, 38.60
    out.append(
        f"Exercise 1.5: A=100->112, S(0)=34, quoted F={fmt(quoted)}, "
        f"fair F={fmt(forward.forward_price(s0, a0, a1))}"
    )
    arb = forward.forward_arbitrage(s0, a0, a1, quoted)
    if arb is None:
        raise RuntimeError("expected a forward arbitrage in Exercise 1.5")
    out.append(
        f"  arbitrage (x,y,z)=({fmt(arb.x)},{fmt(arb.y)},{fmt(arb.z)}) "
        f"V(0)={fmt(arb.value0(s0, a0))} "
        f"V(1)={fmt(arb.value1(30.0, a1, quoted))}|"
        f"{fmt(arb.value1(40.0, a1, quoted))}|"
        f"{fmt(arb.value1(50.0, a1, quoted))} for S(1)=30|40|50"
    )

    foreign_rate = forward.implied_foreign_rate(1.6, 1.50, 0.05)
    out.append(
        "Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> "
        f"sterling rate {fmt(foreign_rate)}, "
        f"bond paying GBP 100 costs GBP {fmt(100.0 / (1 + foreign_rate))}"
    )


def options(out: list[str]) -> None:
    """Sections 1.6-1.7: options, Exercises 1.7-1.10."""
    # p is left unspecified by the book.
    market = one_step.OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
    s_up, s_down = market.s1(UP), market.s1(DOWN)
    call_payoff, put_payoff = one_step.call_payoff, one_step.put_payoff
    out.append(
        "\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 =="
    )
    out.append(
        option_line(
            market,
            "Call X=100",
            call_payoff(s_up, 100.0),
            call_payoff(s_down, 100.0),
        )
    )
    for strike in (90.0, 110.0):
        out.append(
            option_line(
                market,
                f"Exercise 1.7 call X={strike:.0f}",
                call_payoff(s_up, strike),
                call_payoff(s_down, strike),
            )
        )
    for a1 in (105.0, 115.0):
        market_1_8 = one_step.OneStepMarket(100.0, a1, 100.0, 120.0, 80.0, 0.5)
        out.append(
            option_line(
                market_1_8,
                f"Exercise 1.8 call X=100, A(1)={a1:.0f}",
                call_payoff(s_up, 100.0),
                call_payoff(s_down, 100.0),
            )
        )
    out.append(
        option_line(
            market,
            "Exercise 1.9 put X=100",
            put_payoff(s_up, 100.0),
            put_payoff(s_down, 100.0),
        )
    )

    strike = 100.0
    call_up, call_down = call_payoff(s_up, strike), call_payoff(s_down, strike)
    put_up, put_down = put_payoff(s_up, strike), put_payoff(s_down, strike)
    call0 = market.price(call_up, call_down)
    put0 = market.price(put_up, put_down)
    out.append(
        f"Risk-neutral p*={fmt(market.risk_neutral_prob)} -> "
        f"C(0)={fmt(market.risk_neutral_price(call_up, call_down))} "
        f"P(0)={fmt(market.risk_neutral_price(put_up, put_down))}"
    )
    out.append(
        f"Put-call parity: C(0)-P(0)={fmt(call0 - put0)} "
        f"S(0)-X/(1+r)={fmt(market.s0 - strike / (1 + market.r))}"
    )

    out.append(
        "\n== 1.7 Managing risk with options (section 1.6 market, "
        "budget 1000) =="
    )
    budget = 1000.0
    num_shares, num_calls = budget / market.s0, budget / call0
    out.append(
        f"All in stock: {fmt(num_shares)} shares -> "
        f"V(1)={fmt(num_shares * s_up)}|{fmt(num_shares * s_down)} "
        f"return {fmt(num_shares * s_up / budget - 1)}|"
        f"{fmt(num_shares * s_down / budget - 1)}"
    )
    out.append(
        f"All in calls: {fmt(num_calls)} calls  -> "
        f"V(1)={fmt(num_calls * call_up)}|{fmt(num_calls * call_down)} "
        f"return {fmt(num_calls * call_up / budget - 1)}|"
        f"{fmt(num_calls * call_down / budget - 1)}"
    )
    half_shares, half_calls = budget / 2 / market.s0, budget / 2 / call0
    v_up = half_shares * s_up + half_calls * call_up
    v_down = half_shares * s_down + half_calls * call_down
    out.append(
        f"Exercise 1.10 (50/50): {fmt(half_shares)} shares + "
        f"{fmt(half_calls)} calls -> V(1)={fmt(v_up)}|{fmt(v_down)} "
        f"return {fmt(v_up / budget - 1)}|{fmt(v_down / budget - 1)}"
    )


def hedging(out: list[str]) -> None:
    """Section 1.7: hedging a future purchase with calls, Ex. 1.11-1.12."""
    cost = one_step.hedged_purchase_cost
    market = one_step.OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
    out.append(
        "\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, "
        "call X=100"
    )
    out.append(
        f"C(0)={fmt(market.price(60.0, 0.0))} -> loan repaid at t=1: "
        f"{fmt(market.price(60.0, 0.0) * (1 + market.r))}"
    )
    for num_calls in (0.0, 1.0, 2.0):
        out.append(
            f"Cost of one share with {num_calls:.0f} calls: "
            f"{fmt(cost(market, 100.0, num_calls, UP))}|"
            f"{fmt(cost(market, 100.0, num_calls, DOWN))}"
        )
    for p in (0.25, 0.5, 0.75):
        market_p = one_step.OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
        sigma_without = market_p.stddev(
            cost(market_p, 100.0, 0.0, UP), cost(market_p, 100.0, 0.0, DOWN)
        )
        sigma_with = market_p.stddev(
            cost(market_p, 100.0, 1.0, UP), cost(market_p, 100.0, 1.0, DOWN)
        )
        out.append(
            f"Exercise 1.11/1.12 p={fmt(p)}: "
            f"sigma without option={fmt(sigma_without)} "
            f"with option={fmt(sigma_with)} "
            f"ratio={fmt(sigma_with / sigma_without)}"
        )


def report() -> str:
    """Returns the chapter's worked examples as text."""
    out: list[str] = []
    returns_and_portfolios(out)
    currency_arbitrage(out)
    binomial_tree(out)
    risk_and_return(out)
    forwards(out)
    options(out)
    hedging(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
