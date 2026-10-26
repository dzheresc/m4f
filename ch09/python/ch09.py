#!/usr/bin/env python3
"""Chapter 9 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
from black_scholes import bs_greeks, bs_price, norm_inv
from hedging import Position, delta_greek_hedge, delta_hedge, portfolio_value, rebalance_path, total_greek
from risk import (Lognormal, Strategy, all_in_calls, all_in_stock, breakeven, bull_spread, expected_return,
                  initial_cost, mix, prob_loss, put_hedged_holding, unhedged_holding, value_at_risk)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 9: Financial Engineering (illustrative numbers, book formulas)")

    S0, X, r, sigma, T = 100.0, 100.0, 0.05, 0.2, 0.5
    moves = [90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0]

    line("\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 r=0.05 sigma=0.2 ==")
    written = [Position(-1000.0, X, T, True)]
    C = bs_price(S0, X, r, sigma, T, True)
    h = delta_hedge(written, S0, r, sigma)
    line(f"Call price C={f(C)}, delta={f(bs_greeks(S0, X, r, sigma, T, True).delta)}")
    line(f"Premium received {f(1000.0 * C)}; buy {f(h.shares)} shares, money market {f(h.money)} (negative = borrow)")
    naked = Position(-1000.0, X, T, True)
    line("Value of the position right after a jump in S (t=0): naked writer vs delta hedged")
    for S in moves:
        naked_v = 1000.0 * C - 1000.0 * bs_price(S, X, r, sigma, T, True)
        line(f"S={f(S)}: naked {f(naked_v)} hedged {f(portfolio_value(h, S, 0.0, r, sigma))}")
    line("Same jumps one week later (t=1/52), no rebalancing:")
    line("  " + " ".join(f"S={f(S)}:{f(portfolio_value(h, S, 1 / 52, r, sigma))}" for S in moves))
    del naked

    line("\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each week ==")
    path = [100.0, 103.0, 101.0, 98.0, 102.0, 104.0]
    for st in rebalance_path(written, path, 1 / 52, r, sigma):
        line(f"t={f(st.t)} S={f(st.S)}: shares={f(st.shares)} money={f(st.money)} value={f(st.value)}")

    line("\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 ==")
    for name, is_call in (("call", True), ("put", False)):
        g = bs_greeks(S0, X, r, sigma, T, is_call)
        line(f"{name}: price={f(g.price)} delta={f(g.delta)} gamma={f(g.gamma)} theta={f(g.theta)} "
             f"vega={f(g.vega)} rho={f(g.rho)}")
    line("Position Greeks of the written calls: " + " ".join(
        f"{n}={f(total_greek(written, S0, r, sigma, n))}" for n in ("delta", "gamma", "theta", "vega", "rho")))

    line("\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5 ==")
    extra = Position(0.0, 110.0, T, True)
    hg = delta_greek_hedge(written, extra, S0, r, sigma, "gamma")
    hv = delta_greek_hedge(written, extra, S0, r, sigma, "vega")
    for label, hh in (("delta", h), ("delta-gamma", hg), ("delta-vega", hv)):
        opt2 = hh.options[1].qty if len(hh.options) > 1 else 0.0
        line(f"{label}: shares={f(hh.shares)} calls X=110: {f(opt2)} money={f(hh.money)} "
             f"gamma={f(total_greek(hh.options, S0, r, sigma, 'gamma'))} vega={f(total_greek(hh.options, S0, r, sigma, 'vega'))}")
    line("Value after an immediate jump in S:")
    for S in moves:
        line(f"S={f(S)}: delta {f(portfolio_value(h, S, 0.0, r, sigma))} "
             f"delta-gamma {f(portfolio_value(hg, S, 0.0, r, sigma))} delta-vega {f(portfolio_value(hv, S, 0.0, r, sigma))}")
    line("Value after an immediate change in sigma (S unchanged):")
    for sg in (0.15, 0.2, 0.25, 0.3):
        line(f"sigma={f(sg)}: delta {f(portfolio_value(h, S0, 0.0, r, sg))} "
             f"delta-gamma {f(portfolio_value(hg, S0, 0.0, r, sg))} delta-vega {f(portfolio_value(hv, S0, 0.0, r, sg))}")

    line("\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==")
    for label, hh in (("delta", h), ("delta-gamma", hg), ("delta-vega", hv)):
        line(f"{label}: {f(portfolio_value(hh, 97.0, 1 / 52, r, 0.22))}")
    line("Theta: value change over one week with S and sigma unchanged")
    for label, hh in (("delta", h), ("delta-gamma", hg), ("delta-vega", hv)):
        line(f"{label}: {f(portfolio_value(hh, S0, 1 / 52, r, sigma))}  (theta x dt = "
             f"{f(total_greek(hh.options, S0, r, sigma, 'theta') / 52)})")

    line("\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 (lognormal) ==")
    model = Lognormal(100.0, 0.08, 0.3, 0.5)
    stock = unhedged_holding(1.0)
    line(f"E S(T)={f(model.expected_stock())}")
    for conf in (0.9, 0.95, 0.99):
        line(f"confidence {f(conf)}: z={f(norm_inv(1 - conf))} S quantile={f(model.quantile(1 - conf))} "
             f"VaR={f(value_at_risk(stock, model, r, sigma, conf))}")
    line("Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:")
    for Xp in (85.0, 90.0, 95.0, 100.0):
        st = put_hedged_holding(f"put X={int(Xp)}", 1.0, Xp)
        line(f"X={f(Xp)}: put price={f(bs_price(100.0, Xp, r, 0.3, 0.5, False))} initial value={f(initial_cost(st, 100.0, r, 0.3, 0.5))} "
             f"VaR={f(value_at_risk(st, model, r, 0.3, 0.95))} E V(T)={f(st.expected_value(model, r))} "
             f"expected return={f(expected_return(st, model, r, 0.3))}")
    half = put_hedged_holding("half hedged X=95", 1.0, 95.0, 0.5)
    line(f"Half hedged X=95: VaR={f(value_at_risk(half, model, r, 0.3, 0.95))} "
         f"expected return={f(expected_return(half, model, r, 0.3))}")

    line("\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) target 100000 ==")
    big = unhedged_holding(10000.0)
    line(f"Unhedged: VaR={f(value_at_risk(big, model, r, 0.3, 0.95))} expected return={f(expected_return(big, model, r, 0.3))}")
    chosen: Strategy | None = None
    for Xp in (80.0, 85.0, 90.0, 95.0, 100.0, 105.0):
        st = put_hedged_holding(f"put X={int(Xp)}", 10000.0, Xp)
        var = value_at_risk(st, model, r, 0.3, 0.95)
        cost = 10000.0 * bs_price(100.0, Xp, r, 0.3, 0.5, False)
        line(f"puts X={f(Xp)}: cost={f(cost)} VaR={f(var)} expected return={f(expected_return(st, model, r, 0.3))}")
        if chosen is None and var <= 100000.0:
            chosen = st
    assert chosen is not None
    line(f"Cheapest hedge meeting the target: {chosen.name}")

    line("\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 T=0.5, r=0.05 ==")
    spec = Lognormal(100.0, 0.10, 0.25, 0.5)
    sS = all_in_stock(10000.0, 100.0)
    c100 = all_in_calls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.5)
    c110 = all_in_calls("calls X=110", 10000.0, 100.0, 110.0, r, 0.25, 0.5)
    half = mix("half stock, half calls X=100", sS, 0.5, c100, 0.5)
    strategies = [sS, c100, c110, half]
    line(f"C(X=100)={f(bs_price(100.0, 100.0, r, 0.25, 0.5, True))} C(X=110)={f(bs_price(100.0, 110.0, r, 0.25, 0.5, True))}")
    for st in strategies:
        line(f"{st.name}: shares={f(st.shares)} options={fl([l.qty for l in st.legs])}")
    scen = [80.0, 90.0, 100.0, 110.0, 120.0, 130.0]
    line("Terminal value by scenario S(T)=" + fl(scen) + ":")
    for st in strategies:
        line(f"{st.name}: {fl([st.terminal_value(S, r, 0.5) for S in scen])}")
    line("Expected return, probability of loss, breakeven S(T), VaR at 95%:")
    for st in strategies:
        line(f"{st.name}: E ret={f(expected_return(st, spec, r, 0.25))} P(loss)={f(prob_loss(st, spec, r, 0.25))} "
             f"breakeven={f(breakeven(st, spec, r, 0.25))} VaR={f(value_at_risk(st, spec, r, 0.25, 0.95))}")

    line("\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months (m=0.4 sigma=0.25 T=0.25) ==")
    bull = Lognormal(100.0, 0.4, 0.25, 0.25)
    sS = all_in_stock(10000.0, 100.0)
    c100 = all_in_calls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.25)
    bs = bull_spread("bull spread 100/115", 10000.0, 100.0, 100.0, 115.0, r, 0.25, 0.25)
    line(f"E S(T)={f(bull.expected_stock())} P(S(T)>115)={f(1 - bull.cdf(115.0))}")
    for st in (sS, c100, bs):
        line(f"{st.name}: options={fl([l.qty for l in st.legs])} E ret={f(expected_return(st, bull, r, 0.25))} "
             f"P(loss)={f(prob_loss(st, bull, r, 0.25))} VaR={f(value_at_risk(st, bull, r, 0.25, 0.95))} "
             f"value at S(T)=115: {f(st.terminal_value(115.0, r, 0.25))}")
    line("If instead the stock stays flat (S(T)=100): " + " ".join(
        f"{st.name}={f(st.terminal_value(100.0, r, 0.25))}" for st in (sS, c100, bs)))
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
