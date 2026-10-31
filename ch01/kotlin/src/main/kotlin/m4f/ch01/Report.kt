package m4f.ch01

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double, scale: Int = 4): String =
    BigDecimal(v + 0.0).setScale(scale, RoundingMode.HALF_EVEN).toPlainString()

private fun fmt6(v: Double): String = fmt(v, scale = 6)

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

private fun StringBuilder.portfolioLine(market: OneStepMarket, name: String, portfolio: Portfolio) {
    val kUp = market.portfolioReturn(portfolio, State.UP)
    val kDown = market.portfolioReturn(portfolio, State.DOWN)
    line(
        "$name: x=${fmt(portfolio.x)} y=${fmt(portfolio.y)} " +
            "V(0)=${fmt(market.value0(portfolio))} " +
            "V(1)=${fmt(market.value1(portfolio, State.UP))}|" +
            "${fmt(market.value1(portfolio, State.DOWN))} " +
            "K_V=${fmt(kUp)}|${fmt(kDown)}",
    )
}

private fun StringBuilder.riskLine(market: OneStepMarket, name: String, portfolio: Portfolio) {
    val kUp = market.portfolioReturn(portfolio, State.UP)
    val kDown = market.portfolioReturn(portfolio, State.DOWN)
    line(
        "$name: x=${fmt(portfolio.x)} y=${fmt(portfolio.y)} " +
            "V(1)=${fmt(market.value1(portfolio, State.UP))}|" +
            "${fmt(market.value1(portfolio, State.DOWN))} " +
            "K_V=${fmt(kUp)}|${fmt(kDown)} " +
            "E(K_V)=${fmt(market.expect(kUp, kDown))} " +
            "sigma_V=${fmt(market.stddev(kUp, kDown))}",
    )
}

private fun StringBuilder.optionLine(
    market: OneStepMarket,
    name: String,
    dUp: Double,
    dDown: Double,
) {
    val replica = market.replicate(dUp, dDown)
    line(
        "$name: payoff ${fmt(dUp)}|${fmt(dDown)} " +
            "replicate x=${fmt(replica.x)} y=${fmt(replica.y)} " +
            "price=${fmt(market.price(dUp, dDown))}",
    )
}

private fun StringBuilder.arbitrageLines(market: OneStepMarket) {
    val free = if (market.isArbitrageFree) "yes" else "no"
    line(
        "d < r < u: ${fmt(market.d)} < ${fmt(market.r)} < ${fmt(market.u)} " +
            "-> arbitrage-free: $free",
    )
    val arb = market.arbitrage() ?: return
    line(
        "Arbitrage: x=${fmt(arb.x)} y=${fmt(arb.y)} V(0)=${fmt(market.value0(arb))} " +
            "V(1)=${fmt(market.value1(arb, State.UP))}|${fmt(market.value1(arb, State.DOWN))}",
    )
}

/** Examples 1.1-1.2, Exercises 1.1-1.2 and the no-arbitrage check. */
private fun StringBuilder.returnsAndPortfolios() {
    // The book leaves p unspecified (0 < p < 1); it does not affect anything printed here.
    val ex = OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)

    line("== Example 1.1: returns ==")
    line(
        "A(0)=${fmt(ex.a0)} A(1)=${fmt(ex.a1)} S(0)=${fmt(ex.s0)} " +
            "S(1)=${fmt(ex.sUp)}|${fmt(ex.sDown)}",
    )
    line("K_A=${fmt(ex.r)} K_S=${fmt(ex.u)}|${fmt(ex.d)}")

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
}

/** Exercise 1.3. */
private fun StringBuilder.currencyArbitrage() {
    line("\n== Exercise 1.3: currency dealers A (New York) and B (London) ==")
    val fx = FxMarket(
        listOf(
            FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
            FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
            FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
            FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
        ),
    )
    val cycle = fx.bestCycle("USD")
    line("Best round trip: " + (listOf("USD") + cycle.legs.map { it.to }).joinToString(" -> "))
    for (leg in cycle.legs) {
        line("  ${leg.from} -> ${leg.to} with dealer ${leg.dealer}: rate ${fmt6(leg.rate)}")
    }
    val verdict =
        if (cycle.growth > 1) "arbitrage (borrow USD, run the cycle, repay)" else "no arbitrage"
    line("1 USD -> ${fmt6(cycle.growth)} USD: $verdict")
}

/** Example 1.4 / Figure 1.1 and the proof of Proposition 1.1. */
private fun StringBuilder.binomialTree() {
    // p is left unspecified by the book.
    val market = OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
    val sUp = market.s1(State.UP)
    val sDown = market.s1(State.DOWN)
    line("\n== Example 1.4: one-step binomial tree (Figure 1.1) ==")
    line(
        "A(0)=${fmt(market.a0)} A(1)=${fmt(market.a1)} S(0)=${fmt(market.s0)} " +
            "S(1)=${fmt(sUp)}|${fmt(sDown)}",
    )
    line("K_A=${fmt(market.r)} K_S=${fmt(market.u)}|${fmt(market.d)}")
    val holds = if (sDown < market.a1 && market.a1 < sUp) "holds" else "fails"
    line(
        "Proposition 1.1 (S(0)=A(0)): S^d < A(1) < S^u: " +
            "${fmt(sDown)} < ${fmt(market.a1)} < ${fmt(sUp)} -> $holds",
    )
    // Proof of Proposition 1.1: the two violating cases (illustrative S(1) values).
    val cases = listOf(Triple(130.0, 110.0, "A(1) <= S^d"), Triple(110.0, 90.0, "A(1) >= S^u"))
    for ((up, down, label) in cases) {
        val bad = OneStepMarket(100.0, 110.0, 100.0, up, down, 0.5)
        val arb = checkNotNull(bad.arbitrage()) { "expected an arbitrage in case $label" }
        line(
            "  proof, case $label (S(1)=${fmt(up)}|${fmt(down)}): " +
                "x=${fmt(arb.x)} y=${fmt(arb.y)} V(0)=${fmt(bad.value0(arb))} " +
                "V(1)=${fmt(bad.value1(arb, State.UP))}|${fmt(bad.value1(arb, State.DOWN))}",
        )
    }
    arbitrageLines(market)
}

/** Section 1.4 and Exercise 1.4: risk and return, $10,000 to invest. */
private fun StringBuilder.riskAndReturn() {
    val market = OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)
    line("\n== 1.4 Risk and return: A=100->110, S(0)=80, S(1)=100|60, p=0.8, budget 10000 ==")
    riskLine(market, "Mixed", Portfolio(50.0, 60.0))
    riskLine(market, "Bonds only", Portfolio(0.0, 100.0))
    riskLine(market, "Stock only", Portfolio(125.0, 0.0))
    riskLine(market, "Exercise 1.4 (50/50)", market.split(10000.0, 0.5))
}

/** Section 1.5: Example 1.5, Proposition 1.2, Exercises 1.5-1.6. */
private fun StringBuilder.forwards() {
    line("\n== 1.5 Forward contracts ==")
    line("Example 1.5: F=${fmt(80.0)}")
    for (s1 in listOf(84.0, 75.0)) {
        line(
            "  S(1)=${fmt(s1)}: long payoff ${fmt(longForwardPayoff(s1, 80.0))}, " +
                "short payoff ${fmt(shortForwardPayoff(s1, 80.0))}",
        )
    }
    line(
        "Proposition 1.2: S(0)=50 A(0)=100 A(1)=110 -> F = S(0)(1+r) = " +
            fmt(forwardPrice(50.0, 100.0, 110.0)),
    )

    val s0 = 34.0
    val a0 = 100.0
    val a1 = 112.0
    val quoted = 38.60
    line(
        "Exercise 1.5: A=100->112, S(0)=34, quoted F=${fmt(quoted)}, " +
            "fair F=${fmt(forwardPrice(s0, a0, a1))}",
    )
    val arb = checkNotNull(forwardArbitrage(s0, a0, a1, quoted))
    line(
        "  arbitrage (x,y,z)=(${fmt(arb.x)},${fmt(arb.y)},${fmt(arb.z)}) " +
            "V(0)=${fmt(arb.value0(s0, a0))} " +
            "V(1)=${fmt(arb.value1(30.0, a1, quoted))}|${fmt(arb.value1(40.0, a1, quoted))}|" +
            "${fmt(arb.value1(50.0, a1, quoted))} for S(1)=30|40|50",
    )

    val foreignRate = impliedForeignRate(1.6, 1.50, 0.05)
    line(
        "Exercise 1.6: A=100->105 USD, S(0)=1.6 USD/GBP, F=1.50 -> " +
            "sterling rate ${fmt(foreignRate)}, " +
            "bond paying GBP 100 costs GBP ${fmt(100.0 / (1 + foreignRate))}",
    )
}

/** Sections 1.6-1.7: options, Exercises 1.7-1.10. */
private fun StringBuilder.options() {
    // p is left unspecified by the book.
    val market = OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
    val sUp = market.s1(State.UP)
    val sDown = market.s1(State.DOWN)
    line("\n== 1.6 Call and put options: A=100->110, S(0)=100, S(1)=120|80 ==")
    optionLine(market, "Call X=100", callPayoff(sUp, 100.0), callPayoff(sDown, 100.0))
    for (strike in listOf(90.0, 110.0)) {
        optionLine(
            market,
            "Exercise 1.7 call X=${strike.toInt()}",
            callPayoff(sUp, strike),
            callPayoff(sDown, strike),
        )
    }
    for (a1 in listOf(105.0, 115.0)) {
        val market18 = OneStepMarket(100.0, a1, 100.0, 120.0, 80.0, 0.5)
        optionLine(
            market18,
            "Exercise 1.8 call X=100, A(1)=${a1.toInt()}",
            callPayoff(sUp, 100.0),
            callPayoff(sDown, 100.0),
        )
    }
    optionLine(market, "Exercise 1.9 put X=100", putPayoff(sUp, 100.0), putPayoff(sDown, 100.0))

    val strike = 100.0
    val callUp = callPayoff(sUp, strike)
    val callDown = callPayoff(sDown, strike)
    val putUp = putPayoff(sUp, strike)
    val putDown = putPayoff(sDown, strike)
    val call0 = market.price(callUp, callDown)
    val put0 = market.price(putUp, putDown)
    line(
        "Risk-neutral p*=${fmt(market.riskNeutralProb)} -> " +
            "C(0)=${fmt(market.riskNeutralPrice(callUp, callDown))} " +
            "P(0)=${fmt(market.riskNeutralPrice(putUp, putDown))}",
    )
    line(
        "Put-call parity: C(0)-P(0)=${fmt(call0 - put0)} " +
            "S(0)-X/(1+r)=${fmt(market.s0 - strike / (1 + market.r))}",
    )

    line("\n== 1.7 Managing risk with options (section 1.6 market, budget 1000) ==")
    val budget = 1000.0
    val numShares = budget / market.s0
    val numCalls = budget / call0
    line(
        "All in stock: ${fmt(numShares)} shares -> " +
            "V(1)=${fmt(numShares * sUp)}|${fmt(numShares * sDown)} " +
            "return ${fmt(numShares * sUp / budget - 1)}|${fmt(numShares * sDown / budget - 1)}",
    )
    line(
        "All in calls: ${fmt(numCalls)} calls  -> " +
            "V(1)=${fmt(numCalls * callUp)}|${fmt(numCalls * callDown)} " +
            "return ${fmt(numCalls * callUp / budget - 1)}|" +
            fmt(numCalls * callDown / budget - 1),
    )
    val halfShares = budget / 2 / market.s0
    val halfCalls = budget / 2 / call0
    val vUp = halfShares * sUp + halfCalls * callUp
    val vDown = halfShares * sDown + halfCalls * callDown
    line(
        "Exercise 1.10 (50/50): ${fmt(halfShares)} shares + ${fmt(halfCalls)} calls -> " +
            "V(1)=${fmt(vUp)}|${fmt(vDown)} " +
            "return ${fmt(vUp / budget - 1)}|${fmt(vDown / budget - 1)}",
    )
}

/** Section 1.7: hedging a future purchase with calls (strike 100), Exercises 1.11-1.12. */
private fun StringBuilder.hedging() {
    val market = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
    line("\nHedging a future purchase: A=100->110, S(0)=100, S(1)=160|40, call X=100")
    line(
        "C(0)=${fmt(market.price(60.0, 0.0))} -> " +
            "loan repaid at t=1: ${fmt(market.price(60.0, 0.0) * (1 + market.r))}",
    )
    for (numCalls in listOf(0.0, 1.0, 2.0)) {
        line(
            "Cost of one share with ${numCalls.toInt()} calls: " +
                "${fmt(hedgedPurchaseCost(market, 100.0, numCalls, State.UP))}|" +
                fmt(hedgedPurchaseCost(market, 100.0, numCalls, State.DOWN)),
        )
    }
    for (p in listOf(0.25, 0.5, 0.75)) {
        val marketP = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
        val sigmaWithout = marketP.stddev(
            hedgedPurchaseCost(marketP, 100.0, 0.0, State.UP),
            hedgedPurchaseCost(marketP, 100.0, 0.0, State.DOWN),
        )
        val sigmaWith = marketP.stddev(
            hedgedPurchaseCost(marketP, 100.0, 1.0, State.UP),
            hedgedPurchaseCost(marketP, 100.0, 1.0, State.DOWN),
        )
        line(
            "Exercise 1.11/1.12 p=${fmt(p)}: sigma without option=${fmt(sigmaWithout)} " +
                "with option=${fmt(sigmaWith)} ratio=${fmt(sigmaWith / sigmaWithout)}",
        )
    }
}

/** The chapter's worked examples as text. */
fun report(): String = buildString {
    returnsAndPortfolios()
    currencyArbitrage()
    binomialTree()
    riskAndReturn()
    forwards()
    options()
    hedging()
}
