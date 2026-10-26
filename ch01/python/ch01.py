#!/usr/bin/env python3
"""Chapter 1 worked examples. Output is byte-identical to the C++ and Kotlin versions."""
from forward import (forward_arbitrage, forward_price, implied_foreign_rate,
                     long_forward_payoff, short_forward_payoff)
from fx_arbitrage import FxMarket, FxQuote
from one_step import (OneStepMarket, Portfolio, State, call_payoff, hedged_purchase_cost,
                      put_payoff)

UP, DOWN = State.UP, State.DOWN


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def portfolio_line(m: OneStepMarket, name: str, P: Portfolio) -> str:
    ku, kd = m.portfolio_return(P, UP), m.portfolio_return(P, DOWN)
    return (f"{name}: x={f(P.x)} y={f(P.y)} V(0)={f(m.value0(P))} "
            f"V(1)={f(m.value1(P, UP))}|{f(m.value1(P, DOWN))} "
            f"K_V={f(ku)}|{f(kd)}")


def option_line(m: OneStepMarket, name: str, Du: float, Dd: float) -> str:
    R = m.replicate(Du, Dd)
    return (f"{name}: payoff {f(Du)}|{f(Dd)} replicate x={f(R.x)} y={f(R.y)} "
            f"price={f(m.price(Du, Dd))}")


def risk_line(m: OneStepMarket, name: str, P: Portfolio) -> str:
    ku, kd = m.portfolio_return(P, UP), m.portfolio_return(P, DOWN)
    return (f"{name}: x={f(P.x)} y={f(P.y)} V(1)={f(m.value1(P, UP))}|{f(m.value1(P, DOWN))} "
            f"K_V={f(ku)}|{f(kd)} E(K_V)={f(m.expect(ku, kd))} sigma_V={f(m.stddev(ku, kd))}")


def arbitrage_lines(m: OneStepMarket) -> list[str]:
    lines = [f"d < r < u: {f(m.d)} < {f(m.r)} < {f(m.u)} -> arbitrage-free: "
             f"{'yes' if m.is_arbitrage_free else 'no'}"]
    if (P := m.arbitrage()) is not None:
        lines.append(f"Arbitrage: x={f(P.x)} y={f(P.y)} V(0)={f(m.value0(P))} "
                     f"V(1)={f(m.value1(P, UP))}|{f(m.value1(P, DOWN))}")
    return lines


def report() -> str:
    out: list[str] = []
    line = out.append

    # Examples 1.1-1.2. The book leaves p unspecified (0 < p < 1); it does not
    # affect anything printed here.
    ex = OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)

    line("== Example 1.1: returns ==")
    line(f"A(0)={f(ex.A0)} A(1)={f(ex.A1)} S(0)={f(ex.S0)} S(1)={f(ex.Su)}|{f(ex.Sd)}")
    line(f"K_A={f(ex.r)} K_S={f(ex.u)}|{f(ex.d)}")

    line("\n== Example 1.2: portfolio value and return ==")
    line(portfolio_line(ex, "Portfolio", Portfolio(20.0, 10.0)))

    # Exercises 1.1-1.2 (again p is left unspecified).
    exr = OneStepMarket(90.0, 100.0, 25.0, 30.0, 20.0, 0.5)
    line("\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 ==")
    line(portfolio_line(exr, "Portfolio", Portfolio(10.0, 15.0)))

    line("\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==")
    line(portfolio_line(exr, "Solution", exr.replicate(1160.0, 1040.0)))

    line("\n== No-arbitrage check on the Example 1.1 market ==")
    out.extend(arbitrage_lines(ex))

    line("\n== Exercise 1.3: currency dealers A (New York) and B (London) ==")
    fx = FxMarket([FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
                   FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
                   FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
                   FxQuote("B", "USD", "GBP", 0.6299, 0.6375)])
    cyc = fx.best_cycle("USD")
    line("Best round trip: " + " -> ".join(["USD"] + [l.to for l in cyc.legs]))
    for l in cyc.legs:
        line(f"  {l.from_} -> {l.to} with dealer {l.dealer}: rate {l.rate:.6f}")
    line(f"1 USD -> {cyc.growth:.6f} USD: "
         + ("arbitrage (borrow USD, run the cycle, repay)" if cyc.growth > 1 else "no arbitrage"))

    # Example 1.4 / Figure 1.1
    # (p left unspecified by the book).
    m = OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
    Su, Sd = m.S1(UP), m.S1(DOWN)
    line("\n== Example 1.4: one-step binomial tree (Figure 1.1) ==")
    line(f"A(0)={f(m.A0)} A(1)={f(m.A1)} S(0)={f(m.S0)} S(1)={f(Su)}|{f(Sd)}")
    line(f"K_A={f(m.r)} K_S={f(m.u)}|{f(m.d)}")
    line(f"Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: {f(Sd)} < {f(m.A1)} < {f(Su)} -> "
         + ("holds" if Sd < m.A1 < Su else "fails"))
    # Proof of Proposition 1.1: the two violating cases (illustrative S(1) values).
    for up, down, label in ((130.0, 110.0, "A(1) <= S^d"), (110.0, 90.0, "A(1) >= S^u")):
        bad = OneStepMarket(100.0, 110.0, 100.0, up, down, 0.5)
        P = bad.arbitrage()
        assert P is not None
        line(f"  proof, case {label} (S(1)={f(up)}|{f(down)}): x={f(P.x)} y={f(P.y)} V(0)={f(bad.value0(P))} "
             f"V(1)={f(bad.value1(P, UP))}|{f(bad.value1(P, DOWN))}")
    out.extend(arbitrage_lines(m))


    # Section 1.4: risk and return, $10,000 to invest.
    rr = OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)
    line("\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, budget 10000 ==")
    line(risk_line(rr, "Mixed", Portfolio(50.0, 60.0)))
    line(risk_line(rr, "Bonds only", Portfolio(0.0, 100.0)))
    line(risk_line(rr, "Stock only", Portfolio(125.0, 0.0)))
    line(risk_line(rr, "Exercise 1.4 (50/50)", rr.split(10000.0, 0.5)))

    line("\n== 1.5 Forward contracts ==")
    line(f"Example 1.5: F={f(80.0)}")
    for S1 in (84.0, 75.0):
        line(f"  S(1)={f(S1)}: long payoff {f(long_forward_payoff(S1, 80.0))}, "
             f"short payoff {f(short_forward_payoff(S1, 80.0))}")
    line(f"Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = {f(forward_price(50.0, 100.0, 110.0))}")
    S0, A0, A1, F = 34.0, 100.0, 112.0, 38.60
    line(f"Exercise 1.5: A=100->112, S(0)=34, quoted F={f(F)}, fair F={f(forward_price(S0, A0, A1))}")
    FP = forward_arbitrage(S0, A0, A1, F)
    assert FP is not None
    line(f"  arbitrage (x,y,z)=({f(FP.x)},{f(FP.y)},{f(FP.z)}) V(0)={f(FP.value0(S0, A0))} "
         f"V(1)={f(FP.value1(30.0, A1, F))}|{f(FP.value1(40.0, A1, F))}|{f(FP.value1(50.0, A1, F))} "
         "for S(1)=30|40|50")
    rf = implied_foreign_rate(1.6, 1.50, 0.05)
    line(f"Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> sterling rate {f(rf)}, "
         f"bond paying GBP 100 costs GBP {f(100.0 / (1 + rf))}")

    # Section 1.6: call and put options (p left unspecified by the book).
    o = OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
    Ou, Od = o.S1(UP), o.S1(DOWN)
    line("\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 ==")
    line(option_line(o, "Call X=100", call_payoff(Ou, 100.0), call_payoff(Od, 100.0)))
    for X in (90.0, 110.0):
        line(option_line(o, f"Exercise 1.7 call X={X:.0f}", call_payoff(Ou, X), call_payoff(Od, X)))
    for A1 in (105.0, 115.0):
        o8 = OneStepMarket(100.0, A1, 100.0, 120.0, 80.0, 0.5)
        line(option_line(o8, f"Exercise 1.8 call X=100, A(1)={A1:.0f}",
                         call_payoff(Ou, 100.0), call_payoff(Od, 100.0)))
    line(option_line(o, "Exercise 1.9 put X=100", put_payoff(Ou, 100.0), put_payoff(Od, 100.0)))
    X = 100.0
    Cu, Cd = call_payoff(Ou, X), call_payoff(Od, X)
    Pu, Pd = put_payoff(Ou, X), put_payoff(Od, X)
    C0, P0 = o.price(Cu, Cd), o.price(Pu, Pd)
    line(f"Risk-neutral p*={f(o.risk_neutral_prob)} -> C(0)={f(o.risk_neutral_price(Cu, Cd))} "
         f"P(0)={f(o.risk_neutral_price(Pu, Pd))}")
    line(f"Put-call parity: C(0)-P(0)={f(C0 - P0)} S(0)-X/(1+r)={f(o.S0 - X / (1 + o.r))}")

    line("\n== 1.7 Managing risk with options (section 1.6 market, budget 1000) ==")
    budget = 1000.0
    nS, nC = budget / o.S0, budget / C0
    line(f"All in stock: {f(nS)} shares -> V(1)={f(nS * Ou)}|{f(nS * Od)} "
         f"return {f(nS * Ou / budget - 1)}|{f(nS * Od / budget - 1)}")
    line(f"All in calls: {f(nC)} calls  -> V(1)={f(nC * Cu)}|{f(nC * Cd)} "
         f"return {f(nC * Cu / budget - 1)}|{f(nC * Cd / budget - 1)}")
    hS, hC = budget / 2 / o.S0, budget / 2 / C0
    Vu, Vd = hS * Ou + hC * Cu, hS * Od + hC * Cd
    line(f"Exercise 1.10 (50/50): {f(hS)} shares + {f(hC)} calls -> V(1)={f(Vu)}|{f(Vd)} "
         f"return {f(Vu / budget - 1)}|{f(Vd / budget - 1)}")

    # Hedging a future purchase with calls (strike 100).
    h = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
    line("\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call X=100")
    line(f"C(0)={f(h.price(60.0, 0.0))} -> loan repaid at t=1: {f(h.price(60.0, 0.0) * (1 + h.r))}")
    for n in (0.0, 1.0, 2.0):
        line(f"Cost of one share with {n:.0f} calls: {f(hedged_purchase_cost(h, 100.0, n, UP))}|"
             f"{f(hedged_purchase_cost(h, 100.0, n, DOWN))}")
    for p in (0.25, 0.5, 0.75):
        hp = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
        s0 = hp.stddev(hedged_purchase_cost(hp, 100.0, 0.0, UP), hedged_purchase_cost(hp, 100.0, 0.0, DOWN))
        s1 = hp.stddev(hedged_purchase_cost(hp, 100.0, 1.0, UP), hedged_purchase_cost(hp, 100.0, 1.0, DOWN))
        line(f"Exercise 1.11/1.12 p={f(p)}: sigma without option={f(s0)} with option={f(s1)} ratio={f(s1 / s0)}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
