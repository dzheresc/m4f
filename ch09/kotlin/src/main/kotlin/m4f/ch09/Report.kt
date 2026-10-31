package m4f.ch09

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. A tiny negative that rounds to zero prints as
// "-0.0000" there, so it does here too.
private fun fmt(v: Double): String {
    val s = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
    return if (v < 0 && !s.startsWith("-")) "-$s" else s
}

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

private fun legQtys(st: Strategy): List<Double> = st.legs.map { it.qty }

// The option position of Section 9.1.
private const val S0 = 100.0
private const val STRIKE = 100.0
private const val RATE = 0.05
private const val SIGMA = 0.2
private const val MATURITY = 0.5
private val MOVES = listOf(90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0)
private val WRITTEN = listOf(Position(-1000.0, STRIKE, MATURITY, true))

/** Section 9.1.1. */
private fun StringBuilder.deltaHedgingSection() {
    line("\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 r=0.05 sigma=0.2 ==")
    val call = bsPrice(S0, STRIKE, RATE, SIGMA, MATURITY, true)
    val h = deltaHedge(WRITTEN, S0, RATE, SIGMA)
    val delta = bsGreeks(S0, STRIKE, RATE, SIGMA, MATURITY, true).delta
    line("Call price C=${fmt(call)}, delta=${fmt(delta)}")
    line(
        "Premium received ${fmt(1000.0 * call)}; buy ${fmt(h.shares)} shares, " +
            "money market ${fmt(h.money)} (negative = borrow)",
    )
    line("Value of the position right after a jump in S (t=0): naked writer vs delta hedged")
    for (s in MOVES) {
        val naked = 1000.0 * call - 1000.0 * bsPrice(s, STRIKE, RATE, SIGMA, MATURITY, true)
        val hedged = portfolioValue(h, s, 0.0, RATE, SIGMA)
        line("S=${fmt(s)}: naked ${fmt(naked)} hedged ${fmt(hedged)}")
    }
    line("Same jumps one week later (t=1/52), no rebalancing:")
    line(
        "  " +
            MOVES.joinToString(" ") { s ->
                "S=${fmt(s)}:${fmt(portfolioValue(h, s, 1.0 / 52, RATE, SIGMA))}"
            },
    )

    line("\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each week ==")
    val path = listOf(100.0, 103.0, 101.0, 98.0, 102.0, 104.0)
    for (st in rebalancePath(WRITTEN, path, 1.0 / 52, RATE, SIGMA)) {
        line(
            "t=${fmt(st.t)} S=${fmt(st.s)}: shares=${fmt(st.shares)} money=${fmt(st.money)} " +
                "value=${fmt(st.value)}",
        )
    }
}

/** Section 9.1.2, the Greek parameters. */
private fun StringBuilder.greeksSection() {
    line("\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 ==")
    for ((name, isCall) in listOf("call" to true, "put" to false)) {
        val g = bsGreeks(S0, STRIKE, RATE, SIGMA, MATURITY, isCall)
        line(
            "$name: price=${fmt(g.price)} delta=${fmt(g.delta)} gamma=${fmt(g.gamma)} " +
                "theta=${fmt(g.theta)} vega=${fmt(g.vega)} rho=${fmt(g.rho)}",
        )
    }
    line(
        "Position Greeks of the written calls: " +
            Greek.entries.joinToString(" ") {
                "${it.label}=${fmt(totalGreek(WRITTEN, S0, RATE, SIGMA, it))}"
            },
    )
}

/** Sections 9.1.2 (hedging) and 9.1.3. */
private fun StringBuilder.greekHedgingSection() {
    line("\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5 ==")
    val extra = Position(0.0, 110.0, MATURITY, true)
    val h = deltaHedge(WRITTEN, S0, RATE, SIGMA)
    val hGamma = deltaGreekHedge(WRITTEN, extra, S0, RATE, SIGMA, Greek.GAMMA)
    val hVega = deltaGreekHedge(WRITTEN, extra, S0, RATE, SIGMA, Greek.VEGA)
    val hedges = listOf("delta" to h, "delta-gamma" to hGamma, "delta-vega" to hVega)
    for ((label, hedge) in hedges) {
        val second = if (hedge.options.size > 1) hedge.options[1].qty else 0.0
        val gamma = totalGreek(hedge.options, S0, RATE, SIGMA, Greek.GAMMA)
        val vega = totalGreek(hedge.options, S0, RATE, SIGMA, Greek.VEGA)
        line(
            "$label: shares=${fmt(hedge.shares)} calls X=110: ${fmt(second)} " +
                "money=${fmt(hedge.money)} gamma=${fmt(gamma)} vega=${fmt(vega)}",
        )
    }
    line("Value after an immediate jump in S:")
    for (s in MOVES) {
        val values = hedges.map { (_, hedge) -> portfolioValue(hedge, s, 0.0, RATE, SIGMA) }
        line(
            "S=${fmt(s)}: delta ${fmt(values[0])} delta-gamma ${fmt(values[1])} " +
                "delta-vega ${fmt(values[2])}",
        )
    }
    line("Value after an immediate change in sigma (S unchanged):")
    for (sigma in listOf(0.15, 0.2, 0.25, 0.3)) {
        val values = hedges.map { (_, hedge) -> portfolioValue(hedge, S0, 0.0, RATE, sigma) }
        line(
            "sigma=${fmt(sigma)}: delta ${fmt(values[0])} delta-gamma ${fmt(values[1])} " +
                "delta-vega ${fmt(values[2])}",
        )
    }

    line("\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==")
    for ((label, hedge) in hedges) {
        line("$label: ${fmt(portfolioValue(hedge, 97.0, 1.0 / 52, RATE, 0.22))}")
    }
    line("Theta: value change over one week with S and sigma unchanged")
    for ((label, hedge) in hedges) {
        val value = portfolioValue(hedge, S0, 1.0 / 52, RATE, SIGMA)
        val theta = totalGreek(hedge.options, S0, RATE, SIGMA, Greek.THETA)
        line("$label: ${fmt(value)}  (theta x dt = ${fmt(theta / 52)})")
    }
}

/** Sections 9.2.1 and 9.2.2. */
private fun StringBuilder.valueAtRiskSection() {
    line("\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 (lognormal) ==")
    val model = Lognormal(100.0, 0.08, 0.3, 0.5)
    val stock = unhedgedHolding(1.0)
    line("E S(T)=${fmt(model.expectedStock())}")
    for (conf in listOf(0.9, 0.95, 0.99)) {
        line(
            "confidence ${fmt(conf)}: z=${fmt(normInv(1 - conf))} " +
                "S quantile=${fmt(model.quantile(1 - conf))} " +
                "VaR=${fmt(valueAtRisk(stock, model, RATE, SIGMA, conf))}",
        )
    }
    line("Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:")
    for (strike in listOf(85.0, 90.0, 95.0, 100.0)) {
        val st = putHedgedHolding("put X=${strike.toInt()}", 1.0, strike)
        line(
            "X=${fmt(strike)}: put price=${fmt(bsPrice(100.0, strike, RATE, 0.3, 0.5, false))} " +
                "initial value=${fmt(initialCost(st, 100.0, RATE, 0.3, 0.5))} " +
                "VaR=${fmt(valueAtRisk(st, model, RATE, 0.3, 0.95))} " +
                "E V(T)=${fmt(st.expectedValue(model, RATE))} " +
                "expected return=${fmt(expectedReturn(st, model, RATE, 0.3))}",
        )
    }
    val half = putHedgedHolding("half hedged X=95", 1.0, 95.0, 0.5)
    line(
        "Half hedged X=95: VaR=${fmt(valueAtRisk(half, model, RATE, 0.3, 0.95))} " +
            "expected return=${fmt(expectedReturn(half, model, RATE, 0.3))}",
    )

    line("\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) target 100000 ==")
    val big = unhedgedHolding(10000.0)
    line(
        "Unhedged: VaR=${fmt(valueAtRisk(big, model, RATE, 0.3, 0.95))} " +
            "expected return=${fmt(expectedReturn(big, model, RATE, 0.3))}",
    )
    var chosen = ""
    for (strike in listOf(80.0, 85.0, 90.0, 95.0, 100.0, 105.0)) {
        val st = putHedgedHolding("put X=${strike.toInt()}", 10000.0, strike)
        val v = valueAtRisk(st, model, RATE, 0.3, 0.95)
        val cost = 10000.0 * bsPrice(100.0, strike, RATE, 0.3, 0.5, false)
        line(
            "puts X=${fmt(strike)}: cost=${fmt(cost)} VaR=${fmt(v)} " +
                "expected return=${fmt(expectedReturn(st, model, RATE, 0.3))}",
        )
        if (chosen.isEmpty() && v <= 100000.0) chosen = st.name
    }
    line("Cheapest hedge meeting the target: $chosen")
}

/** Section 9.3.1. */
private fun StringBuilder.speculatingSection() {
    line("\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 T=0.5, r=0.05 ==")
    val spec = Lognormal(100.0, 0.10, 0.25, 0.5)
    val stock = allInStock(10000.0, 100.0)
    val calls100 = allInCalls("calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.5)
    val calls110 = allInCalls("calls X=110", 10000.0, 100.0, 110.0, RATE, 0.25, 0.5)
    val halfMix = mix("half stock, half calls X=100", stock, 0.5, calls100, 0.5)
    val strategies = listOf(stock, calls100, calls110, halfMix)
    line(
        "C(X=100)=${fmt(bsPrice(100.0, 100.0, RATE, 0.25, 0.5, true))} " +
            "C(X=110)=${fmt(bsPrice(100.0, 110.0, RATE, 0.25, 0.5, true))}",
    )
    for (st in strategies) {
        line("${st.name}: shares=${fmt(st.shares)} options=${fmtList(legQtys(st))}")
    }
    val scenarios = listOf(80.0, 90.0, 100.0, 110.0, 120.0, 130.0)
    line("Terminal value by scenario S(T)=" + fmtList(scenarios) + ":")
    for (st in strategies) {
        line("${st.name}: ${fmtList(scenarios.map { st.terminalValue(it, RATE, 0.5) })}")
    }
    line("Expected return, probability of loss, breakeven S(T), VaR at 95%:")
    for (st in strategies) {
        line(
            "${st.name}: E ret=${fmt(expectedReturn(st, spec, RATE, 0.25))} " +
                "P(loss)=${fmt(probLoss(st, spec, RATE, 0.25))} " +
                "breakeven=${fmt(breakeven(st, spec, RATE, 0.25))} " +
                "VaR=${fmt(valueAtRisk(st, spec, RATE, 0.25, 0.95))}",
        )
    }
}

/** Section 9.3.2. */
private fun StringBuilder.bullCaseSection() {
    line(
        "\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months " +
            "(m=0.4 sigma=0.25 T=0.25) ==",
    )
    val bull = Lognormal(100.0, 0.4, 0.25, 0.25)
    val strategies = listOf(
        allInStock(10000.0, 100.0),
        allInCalls("calls X=100", 10000.0, 100.0, 100.0, RATE, 0.25, 0.25),
        bullSpread("bull spread 100/115", 10000.0, 100.0, 100.0, 115.0, RATE, 0.25, 0.25),
    )
    line("E S(T)=${fmt(bull.expectedStock())} P(S(T)>115)=${fmt(1 - bull.cdf(115.0))}")
    for (st in strategies) {
        line(
            "${st.name}: options=${fmtList(legQtys(st))} " +
                "E ret=${fmt(expectedReturn(st, bull, RATE, 0.25))} " +
                "P(loss)=${fmt(probLoss(st, bull, RATE, 0.25))} " +
                "VaR=${fmt(valueAtRisk(st, bull, RATE, 0.25, 0.95))} " +
                "value at S(T)=115: ${fmt(st.terminalValue(115.0, RATE, 0.25))}",
        )
    }
    line(
        "If instead the stock stays flat (S(T)=100): " +
            strategies.joinToString(" ") {
                "${it.name}=${fmt(it.terminalValue(100.0, RATE, 0.25))}"
            },
    )
}

/** The full chapter 9 report. */
fun report(): String = buildString {
    line("Chapter 9: Financial Engineering (illustrative numbers, book formulas)")
    deltaHedgingSection()
    greeksSection()
    greekHedgingSection()
    valueAtRiskSection()
    speculatingSection()
    bullCaseSection()
}
