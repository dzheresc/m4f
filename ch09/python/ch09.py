#!/usr/bin/env python3
"""Chapter 9 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import black_scholes
import hedging
import risk

# The option position of Section 9.1.
S0, STRIKE, RATE, SIGMA, MATURITY = 100.0, 100.0, 0.05, 0.2, 0.5
MOVES = (90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0)
WRITTEN = [hedging.Position(-1000.0, STRIKE, MATURITY, True)]


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def leg_qtys(st: risk.Strategy) -> list[float]:
    return [leg.qty for leg in st.legs]


def delta_hedging_section(out: list[str]) -> None:
    """Section 9.1.1."""
    out.append(
        "\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 "
        "r=0.05 sigma=0.2 =="
    )
    call = black_scholes.bs_price(S0, STRIKE, RATE, SIGMA, MATURITY, True)
    h = hedging.delta_hedge(WRITTEN, S0, RATE, SIGMA)
    delta = black_scholes.bs_greeks(
        S0, STRIKE, RATE, SIGMA, MATURITY, True
    ).delta
    out.append(f"Call price C={fmt(call)}, delta={fmt(delta)}")
    out.append(
        f"Premium received {fmt(1000.0 * call)}; buy {fmt(h.shares)} shares, "
        f"money market {fmt(h.money)} (negative = borrow)"
    )
    out.append(
        "Value of the position right after a jump in S (t=0): naked writer vs"
        " delta hedged"
    )
    for s in MOVES:
        naked = 1000.0 * call - 1000.0 * black_scholes.bs_price(
            s, STRIKE, RATE, SIGMA, MATURITY, True
        )
        hedged = hedging.portfolio_value(h, s, 0.0, RATE, SIGMA)
        out.append(f"S={fmt(s)}: naked {fmt(naked)} hedged {fmt(hedged)}")
    out.append("Same jumps one week later (t=1/52), no rebalancing:")
    out.append(
        "  "
        + " ".join(
            f"S={fmt(s)}:"
            f"{fmt(hedging.portfolio_value(h, s, 1 / 52, RATE, SIGMA))}"
            for s in MOVES
        )
    )

    out.append(
        "\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each"
        " week =="
    )
    path = [100.0, 103.0, 101.0, 98.0, 102.0, 104.0]
    for st in hedging.rebalance_path(WRITTEN, path, 1 / 52, RATE, SIGMA):
        out.append(
            f"t={fmt(st.t)} S={fmt(st.s)}: shares={fmt(st.shares)} "
            f"money={fmt(st.money)} value={fmt(st.value)}"
        )


def greeks_section(out: list[str]) -> None:
    """Section 9.1.2, the Greek parameters."""
    out.append(
        "\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 =="
    )
    for name, is_call in (("call", True), ("put", False)):
        g = black_scholes.bs_greeks(S0, STRIKE, RATE, SIGMA, MATURITY, is_call)
        out.append(
            f"{name}: price={fmt(g.price)} delta={fmt(g.delta)} "
            f"gamma={fmt(g.gamma)} theta={fmt(g.theta)} vega={fmt(g.vega)} "
            f"rho={fmt(g.rho)}"
        )
    out.append(
        "Position Greeks of the written calls: "
        + " ".join(
            f"{n}={fmt(hedging.total_greek(WRITTEN, S0, RATE, SIGMA, n))}"
            for n in ("delta", "gamma", "theta", "vega", "rho")
        )
    )


def greek_hedging_section(out: list[str]) -> None:
    """Sections 9.1.2 (hedging) and 9.1.3."""
    out.append(
        "\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5"
        " =="
    )
    extra = hedging.Position(0.0, 110.0, MATURITY, True)
    h = hedging.delta_hedge(WRITTEN, S0, RATE, SIGMA)
    h_gamma = hedging.delta_greek_hedge(
        WRITTEN, extra, S0, RATE, SIGMA, "gamma"
    )
    h_vega = hedging.delta_greek_hedge(WRITTEN, extra, S0, RATE, SIGMA, "vega")
    hedges = (("delta", h), ("delta-gamma", h_gamma), ("delta-vega", h_vega))
    for label, hedge in hedges:
        second = hedge.options[1].qty if len(hedge.options) > 1 else 0.0
        gamma = hedging.total_greek(hedge.options, S0, RATE, SIGMA, "gamma")
        vega = hedging.total_greek(hedge.options, S0, RATE, SIGMA, "vega")
        out.append(
            f"{label}: shares={fmt(hedge.shares)} calls X=110: {fmt(second)} "
            f"money={fmt(hedge.money)} gamma={fmt(gamma)} vega={fmt(vega)}"
        )
    out.append("Value after an immediate jump in S:")
    for s in MOVES:
        values = [
            hedging.portfolio_value(hedge, s, 0.0, RATE, SIGMA)
            for _, hedge in hedges
        ]
        out.append(
            f"S={fmt(s)}: delta {fmt(values[0])} delta-gamma {fmt(values[1])}"
            f" delta-vega {fmt(values[2])}"
        )
    out.append("Value after an immediate change in sigma (S unchanged):")
    for sigma in (0.15, 0.2, 0.25, 0.3):
        values = [
            hedging.portfolio_value(hedge, S0, 0.0, RATE, sigma)
            for _, hedge in hedges
        ]
        out.append(
            f"sigma={fmt(sigma)}: delta {fmt(values[0])} "
            f"delta-gamma {fmt(values[1])} delta-vega {fmt(values[2])}"
        )

    out.append("\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==")
    for label, hedge in hedges:
        value = hedging.portfolio_value(hedge, 97.0, 1 / 52, RATE, 0.22)
        out.append(f"{label}: {fmt(value)}")
    out.append("Theta: value change over one week with S and sigma unchanged")
    for label, hedge in hedges:
        value = hedging.portfolio_value(hedge, S0, 1 / 52, RATE, SIGMA)
        theta = hedging.total_greek(hedge.options, S0, RATE, SIGMA, "theta")
        out.append(f"{label}: {fmt(value)}  (theta x dt = {fmt(theta / 52)})")


def value_at_risk_section(out: list[str]) -> None:
    """Sections 9.2.1 and 9.2.2."""
    out.append(
        "\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5"
        " (lognormal) =="
    )
    model = risk.Lognormal(100.0, 0.08, 0.3, 0.5)
    stock = risk.unhedged_holding(1.0)
    out.append(f"E S(T)={fmt(model.expected_stock())}")
    for conf in (0.9, 0.95, 0.99):
        var = risk.value_at_risk(stock, model, RATE, SIGMA, conf)
        out.append(
            f"confidence {fmt(conf)}: z={fmt(black_scholes.norm_inv(1 - conf))}"
            f" S quantile={fmt(model.quantile(1 - conf))} VaR={fmt(var)}"
        )
    out.append("Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:")
    for strike in (85.0, 90.0, 95.0, 100.0):
        st = risk.put_hedged_holding(f"put X={int(strike)}", 1.0, strike)
        put = black_scholes.bs_price(100.0, strike, RATE, 0.3, 0.5, False)
        out.append(
            f"X={fmt(strike)}: put price={fmt(put)} "
            f"initial value={fmt(risk.initial_cost(st, 100.0, RATE, 0.3, 0.5))}"
            f" VaR={fmt(risk.value_at_risk(st, model, RATE, 0.3, 0.95))} "
            f"E V(T)={fmt(st.expected_value(model, RATE))} "
            f"expected return={fmt(risk.expected_return(st, model, RATE, 0.3))}"
        )
    half = risk.put_hedged_holding("half hedged X=95", 1.0, 95.0, 0.5)
    out.append(
        "Half hedged X=95: "
        f"VaR={fmt(risk.value_at_risk(half, model, RATE, 0.3, 0.95))} "
        f"expected return={fmt(risk.expected_return(half, model, RATE, 0.3))}"
    )

    out.append(
        "\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%)"
        " target 100000 =="
    )
    big = risk.unhedged_holding(10000.0)
    out.append(
        f"Unhedged: VaR={fmt(risk.value_at_risk(big, model, RATE, 0.3, 0.95))}"
        f" expected return={fmt(risk.expected_return(big, model, RATE, 0.3))}"
    )
    chosen = ""
    for strike in (80.0, 85.0, 90.0, 95.0, 100.0, 105.0):
        st = risk.put_hedged_holding(f"put X={int(strike)}", 10000.0, strike)
        var = risk.value_at_risk(st, model, RATE, 0.3, 0.95)
        cost = 10000.0 * black_scholes.bs_price(
            100.0, strike, RATE, 0.3, 0.5, False
        )
        out.append(
            f"puts X={fmt(strike)}: cost={fmt(cost)} VaR={fmt(var)} "
            f"expected return={fmt(risk.expected_return(st, model, RATE, 0.3))}"
        )
        if not chosen and var <= 100000.0:
            chosen = st.name
    out.append(f"Cheapest hedge meeting the target: {chosen}")


def speculating_section(out: list[str]) -> None:
    """Section 9.3.1."""
    out.append(
        "\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25"
        " T=0.5, r=0.05 =="
    )
    spec = risk.Lognormal(100.0, 0.10, 0.25, 0.5)
    stock = risk.all_in_stock(10000.0, 100.0)
    calls100 = risk.all_in_calls(
        "calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.5
    )
    calls110 = risk.all_in_calls(
        "calls X=110", 10000.0, 100.0, 110.0, RATE, 0.25, 0.5
    )
    half_mix = risk.mix(
        "half stock, half calls X=100", stock, 0.5, calls100, 0.5
    )
    strategies = [stock, calls100, calls110, half_mix]
    out.append(
        "C(X=100)="
        f"{fmt(black_scholes.bs_price(100.0, 100.0, RATE, 0.25, 0.5, True))} "
        "C(X=110)="
        f"{fmt(black_scholes.bs_price(100.0, 110.0, RATE, 0.25, 0.5, True))}"
    )
    for st in strategies:
        out.append(
            f"{st.name}: shares={fmt(st.shares)} "
            f"options={fmt_list(leg_qtys(st))}"
        )
    scenarios = [80.0, 90.0, 100.0, 110.0, 120.0, 130.0]
    out.append("Terminal value by scenario S(T)=" + fmt_list(scenarios) + ":")
    for st in strategies:
        values = [st.terminal_value(s, RATE, 0.5) for s in scenarios]
        out.append(f"{st.name}: {fmt_list(values)}")
    out.append(
        "Expected return, probability of loss, breakeven S(T), VaR at 95%:"
    )
    for st in strategies:
        out.append(
            f"{st.name}: "
            f"E ret={fmt(risk.expected_return(st, spec, RATE, 0.25))} "
            f"P(loss)={fmt(risk.prob_loss(st, spec, RATE, 0.25))} "
            f"breakeven={fmt(risk.breakeven(st, spec, RATE, 0.25))} "
            f"VaR={fmt(risk.value_at_risk(st, spec, RATE, 0.25, 0.95))}"
        )


def bull_case_section(out: list[str]) -> None:
    """Section 9.3.2."""
    out.append(
        "\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months"
        " (m=0.4 sigma=0.25 T=0.25) =="
    )
    bull = risk.Lognormal(100.0, 0.4, 0.25, 0.25)
    strategies = (
        risk.all_in_stock(10000.0, 100.0),
        risk.all_in_calls(
            "calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.25
        ),
        risk.bull_spread(
            "bull spread 100/115",
            10000.0,
            100.0,
            100.0,
            115.0,
            RATE,
            0.25,
            0.25,
        ),
    )
    out.append(
        f"E S(T)={fmt(bull.expected_stock())} "
        f"P(S(T)>115)={fmt(1 - bull.cdf(115.0))}"
    )
    for st in strategies:
        out.append(
            f"{st.name}: options={fmt_list(leg_qtys(st))} "
            f"E ret={fmt(risk.expected_return(st, bull, RATE, 0.25))} "
            f"P(loss)={fmt(risk.prob_loss(st, bull, RATE, 0.25))} "
            f"VaR={fmt(risk.value_at_risk(st, bull, RATE, 0.25, 0.95))} "
            f"value at S(T)=115: {fmt(st.terminal_value(115.0, RATE, 0.25))}"
        )
    out.append(
        "If instead the stock stays flat (S(T)=100): "
        + " ".join(
            f"{st.name}={fmt(st.terminal_value(100.0, RATE, 0.25))}"
            for st in strategies
        )
    )


def report() -> str:
    """The full chapter 9 report."""
    out = [
        "Chapter 9: Financial Engineering (illustrative numbers, book formulas)"
    ]
    delta_hedging_section(out)
    greeks_section(out)
    greek_hedging_section(out)
    value_at_risk_section(out)
    speculating_section(out)
    bull_case_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
