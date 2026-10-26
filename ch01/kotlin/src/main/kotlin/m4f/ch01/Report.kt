package m4f.ch01

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
private fun f6(v: Double): String = BigDecimal(v + 0.0).setScale(6, RoundingMode.HALF_EVEN).toPlainString()

private fun StringBuilder.line(s: String) = append(s).append('\n')

private fun StringBuilder.portfolioLine(m: OneStepMarket, name: String, P: Portfolio) {
    val ku = m.portfolioReturn(P, State.Up)
    val kd = m.portfolioReturn(P, State.Down)
    line("$name: x=${f(P.x)} y=${f(P.y)} V(0)=${f(m.value0(P))} " +
        "V(1)=${f(m.value1(P, State.Up))}|${f(m.value1(P, State.Down))} " +
        "K_V=${f(ku)}|${f(kd)}")
}

private fun StringBuilder.arbitrageLines(m: OneStepMarket) {
    line("d < r < u: ${f(m.d)} < ${f(m.r)} < ${f(m.u)} -> arbitrage-free: ${if (m.isArbitrageFree) "yes" else "no"}")
    m.arbitrage()?.let { P ->
        line("Arbitrage: x=${f(P.x)} y=${f(P.y)} V(0)=${f(m.value0(P))} " +
            "V(1)=${f(m.value1(P, State.Up))}|${f(m.value1(P, State.Down))}")
    }
}

private fun StringBuilder.riskLine(m: OneStepMarket, name: String, P: Portfolio) {
    val ku = m.portfolioReturn(P, State.Up)
    val kd = m.portfolioReturn(P, State.Down)
    line("$name: x=${f(P.x)} y=${f(P.y)} V(1)=${f(m.value1(P, State.Up))}|${f(m.value1(P, State.Down))} " +
        "K_V=${f(ku)}|${f(kd)} E(K_V)=${f(m.expect(ku, kd))} sigma_V=${f(m.stddev(ku, kd))}")
}

private fun StringBuilder.optionLine(m: OneStepMarket, name: String, Du: Double, Dd: Double) {
    val R = m.replicate(Du, Dd)
    line("$name: payoff ${f(Du)}|${f(Dd)} replicate x=${f(R.x)} y=${f(R.y)} price=${f(m.price(Du, Dd))}")
}

fun report(): String = buildString {
    // Examples 1.1-1.2. The book leaves p unspecified (0 < p < 1); it does not
    // affect anything printed here.
    val ex = OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)

    line("== Example 1.1: returns ==")
    line("A(0)=${f(ex.A0)} A(1)=${f(ex.A1)} S(0)=${f(ex.S0)} S(1)=${f(ex.Su)}|${f(ex.Sd)}")
    line("K_A=${f(ex.r)} K_S=${f(ex.u)}|${f(ex.d)}")

    line("\n== Example 1.2: portfolio value and return ==")
    portfolioLine(ex, "Portfolio", Portfolio(20.0, 10.0))

    // Exercises 1.1-1.2 (again p is left unspecified).
    val exr = OneStepMarket(90.0, 100.0, 25.0, 30.0, 20.0, 0.5)
    line("\n== Exercise 1.1: A(0)=90 A(1)=100 S(0)=25 S(1)=30|20 ==")
    portfolioLine(exr, "Portfolio", Portfolio(10.0, 15.0))

    line("\n== Exercise 1.2: portfolio with V(1)=1160|1040 ==")
    portfolioLine(exr, "Solution", exr.replicate(1160.0, 1040.0))

    line("\n== No-arbitrage check on the Example 1.1 market ==")
    arbitrageLines(ex)

    line("\n== Exercise 1.3: currency dealers A (New York) and B (London) ==")
    val fx = FxMarket(listOf(
        FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
        FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
        FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
        FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
    ))
    val cyc = fx.bestCycle("USD")
    line("Best round trip: " + (listOf("USD") + cyc.legs.map { it.to }).joinToString(" -> "))
    for (l in cyc.legs) line("  ${l.from} -> ${l.to} with dealer ${l.dealer}: rate ${f6(l.rate)}")
    line("1 USD -> ${f6(cyc.growth)} USD: " +
        if (cyc.growth > 1) "arbitrage (borrow USD, run the cycle, repay)" else "no arbitrage")

    // Example 1.4 / Figure 1.1
    // (p left unspecified by the book).
    val m = OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
    val Su = m.S1(State.Up)
    val Sd = m.S1(State.Down)
    line("\n== Example 1.4: one-step binomial tree (Figure 1.1) ==")
    line("A(0)=${f(m.A0)} A(1)=${f(m.A1)} S(0)=${f(m.S0)} S(1)=${f(Su)}|${f(Sd)}")
    line("K_A=${f(m.r)} K_S=${f(m.u)}|${f(m.d)}")
    line("Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: ${f(Sd)} < ${f(m.A1)} < ${f(Su)} -> " +
        if (Sd < m.A1 && m.A1 < Su) "holds" else "fails")
    // Proof of Proposition 1.1: the two violating cases (illustrative S(1) values).
    for ((up, down, label) in listOf(Triple(130.0, 110.0, "A(1) <= S^d"), Triple(110.0, 90.0, "A(1) >= S^u"))) {
        val bad = OneStepMarket(100.0, 110.0, 100.0, up, down, 0.5)
        val P = bad.arbitrage()!!
        line("  proof, case $label (S(1)=${f(up)}|${f(down)}): x=${f(P.x)} y=${f(P.y)} V(0)=${f(bad.value0(P))} " +
            "V(1)=${f(bad.value1(P, State.Up))}|${f(bad.value1(P, State.Down))}")
    }
    arbitrageLines(m)


    // Section 1.4: risk and return, $10,000 to invest.
    val rr = OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)
    line("\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, budget 10000 ==")
    riskLine(rr, "Mixed", Portfolio(50.0, 60.0))
    riskLine(rr, "Bonds only", Portfolio(0.0, 100.0))
    riskLine(rr, "Stock only", Portfolio(125.0, 0.0))
    riskLine(rr, "Exercise 1.4 (50/50)", rr.split(10000.0, 0.5))

    line("\n== 1.5 Forward contracts ==")
    line("Example 1.5: F=${f(80.0)}")
    for (S1 in listOf(84.0, 75.0))
        line("  S(1)=${f(S1)}: long payoff ${f(longForwardPayoff(S1, 80.0))}, short payoff ${f(shortForwardPayoff(S1, 80.0))}")
    line("Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = ${f(forwardPrice(50.0, 100.0, 110.0))}")
    run {
        val S0 = 34.0; val A0 = 100.0; val A1 = 112.0; val F = 38.60
        line("Exercise 1.5: A=100->112, S(0)=34, quoted F=${f(F)}, fair F=${f(forwardPrice(S0, A0, A1))}")
        val P = forwardArbitrage(S0, A0, A1, F)!!
        line("  arbitrage (x,y,z)=(${f(P.x)},${f(P.y)},${f(P.z)}) V(0)=${f(P.value0(S0, A0))} " +
            "V(1)=${f(P.value1(30.0, A1, F))}|${f(P.value1(40.0, A1, F))}|${f(P.value1(50.0, A1, F))} for S(1)=30|40|50")
    }
    run {
        val rf = impliedForeignRate(1.6, 1.50, 0.05)
        line("Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> sterling rate ${f(rf)}, " +
            "bond paying GBP 100 costs GBP ${f(100.0 / (1 + rf))}")
    }

    // Section 1.6: call and put options (p left unspecified by the book).
    val o = OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
    val Ou = o.S1(State.Up)
    val Od = o.S1(State.Down)
    line("\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 ==")
    optionLine(o, "Call X=100", callPayoff(Ou, 100.0), callPayoff(Od, 100.0))
    for (X in listOf(90.0, 110.0))
        optionLine(o, "Exercise 1.7 call X=${X.toInt()}", callPayoff(Ou, X), callPayoff(Od, X))
    for (A1 in listOf(105.0, 115.0)) {
        val o8 = OneStepMarket(100.0, A1, 100.0, 120.0, 80.0, 0.5)
        optionLine(o8, "Exercise 1.8 call X=100, A(1)=${A1.toInt()}", callPayoff(Ou, 100.0), callPayoff(Od, 100.0))
    }
    optionLine(o, "Exercise 1.9 put X=100", putPayoff(Ou, 100.0), putPayoff(Od, 100.0))
    val X = 100.0
    val Cu = callPayoff(Ou, X); val Cd = callPayoff(Od, X)
    val Pu = putPayoff(Ou, X); val Pd = putPayoff(Od, X)
    val C0 = o.price(Cu, Cd); val P0 = o.price(Pu, Pd)
    line("Risk-neutral p*=${f(o.riskNeutralProb)} -> C(0)=${f(o.riskNeutralPrice(Cu, Cd))} P(0)=${f(o.riskNeutralPrice(Pu, Pd))}")
    line("Put-call parity: C(0)-P(0)=${f(C0 - P0)} S(0)-X/(1+r)=${f(o.S0 - X / (1 + o.r))}")

    line("\n== 1.7 Managing risk with options (section 1.6 market, budget 1000) ==")
    val budget = 1000.0
    val nS = budget / o.S0
    val nC = budget / C0
    line("All in stock: ${f(nS)} shares -> V(1)=${f(nS * Ou)}|${f(nS * Od)} return ${f(nS * Ou / budget - 1)}|${f(nS * Od / budget - 1)}")
    line("All in calls: ${f(nC)} calls  -> V(1)=${f(nC * Cu)}|${f(nC * Cd)} return ${f(nC * Cu / budget - 1)}|${f(nC * Cd / budget - 1)}")
    run {
        val hS = budget / 2 / o.S0
        val hC = budget / 2 / C0
        val Vu = hS * Ou + hC * Cu
        val Vd = hS * Od + hC * Cd
        line("Exercise 1.10 (50/50): ${f(hS)} shares + ${f(hC)} calls -> V(1)=${f(Vu)}|${f(Vd)} return ${f(Vu / budget - 1)}|${f(Vd / budget - 1)}")
    }

    // Hedging a future purchase with calls (strike 100).
    val h = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
    line("\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call X=100")
    line("C(0)=${f(h.price(60.0, 0.0))} -> loan repaid at t=1: ${f(h.price(60.0, 0.0) * (1 + h.r))}")
    for (n in listOf(0.0, 1.0, 2.0))
        line("Cost of one share with ${n.toInt()} calls: ${f(hedgedPurchaseCost(h, 100.0, n, State.Up))}|${f(hedgedPurchaseCost(h, 100.0, n, State.Down))}")
    for (p in listOf(0.25, 0.5, 0.75)) {
        val hp = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
        val s0 = hp.stddev(hedgedPurchaseCost(hp, 100.0, 0.0, State.Up), hedgedPurchaseCost(hp, 100.0, 0.0, State.Down))
        val s1 = hp.stddev(hedgedPurchaseCost(hp, 100.0, 1.0, State.Up), hedgedPurchaseCost(hp, 100.0, 1.0, State.Down))
        line("Exercise 1.11/1.12 p=${f(p)}: sigma without option=${f(s0)} with option=${f(s1)} ratio=${f(s1 / s0)}")
    }
}
