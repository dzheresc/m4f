package m4f.ch09

import java.math.BigDecimal
import java.math.RoundingMode

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations. A tiny
// negative that rounds to zero prints as "-0.0000" there, so it does here too.
private fun f(v: Double): String {
    val s = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
    return if (v < 0 && !s.startsWith("-")) "-$s" else s
}

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

private fun legQtys(st: Strategy): List<Double> = st.legs.map { it.qty }

fun report(): String = buildString {
    line("Chapter 9: Financial Engineering (illustrative numbers, book formulas)")

    val S0 = 100.0
    val X = 100.0
    val r = 0.05
    val sigma = 0.2
    val T = 0.5
    val moves = listOf(90.0, 95.0, 99.0, 100.0, 101.0, 105.0, 110.0)

    line("\n== 9.1.1 Delta hedging: written 1000 calls X=100 T=0.5, S=100 r=0.05 sigma=0.2 ==")
    val written = listOf(Position(-1000.0, X, T, true))
    val C = bsPrice(S0, X, r, sigma, T, true)
    val h = deltaHedge(written, S0, r, sigma)
    line("Call price C=${f(C)}, delta=${f(bsGreeks(S0, X, r, sigma, T, true).delta)}")
    line("Premium received ${f(1000.0 * C)}; buy ${f(h.shares)} shares, money market ${f(h.money)} (negative = borrow)")
    line("Value of the position right after a jump in S (t=0): naked writer vs delta hedged")
    for (S in moves) {
        val nakedV = 1000.0 * C - 1000.0 * bsPrice(S, X, r, sigma, T, true)
        line("S=${f(S)}: naked ${f(nakedV)} hedged ${f(portfolioValue(h, S, 0.0, r, sigma))}")
    }
    line("Same jumps one week later (t=1/52), no rebalancing:")
    line("  " + moves.joinToString(" ") { S -> "S=${f(S)}:${f(portfolioValue(h, S, 1.0 / 52, r, sigma))}" })

    line("\n== 9.1.1 Dynamic delta hedging along a weekly path, rebalanced each week ==")
    val path = listOf(100.0, 103.0, 101.0, 98.0, 102.0, 104.0)
    for (st in rebalancePath(written, path, 1.0 / 52, r, sigma))
        line("t=${f(st.t)} S=${f(st.S)}: shares=${f(st.shares)} money=${f(st.money)} value=${f(st.value)}")

    line("\n== 9.1.2 Greek parameters: S=100 X=100 r=0.05 sigma=0.2 T=0.5 ==")
    for ((name, isCall) in listOf("call" to true, "put" to false)) {
        val g = bsGreeks(S0, X, r, sigma, T, isCall)
        line("$name: price=${f(g.price)} delta=${f(g.delta)} gamma=${f(g.gamma)} theta=${f(g.theta)} " +
            "vega=${f(g.vega)} rho=${f(g.rho)}")
    }
    line("Position Greeks of the written calls: " + Greek.entries.joinToString(" ") {
        "${it.label}=${f(totalGreek(written, S0, r, sigma, it))}"
    })

    line("\n== 9.1.2 Delta-gamma and delta-vega hedging with calls X=110 T=0.5 ==")
    val extra = Position(0.0, 110.0, T, true)
    val hg = deltaGreekHedge(written, extra, S0, r, sigma, Greek.GAMMA)
    val hv = deltaGreekHedge(written, extra, S0, r, sigma, Greek.VEGA)
    val hedges = listOf("delta" to h, "delta-gamma" to hg, "delta-vega" to hv)
    for ((label, hh) in hedges) {
        val opt2 = if (hh.options.size > 1) hh.options[1].qty else 0.0
        line("$label: shares=${f(hh.shares)} calls X=110: ${f(opt2)} money=${f(hh.money)} " +
            "gamma=${f(totalGreek(hh.options, S0, r, sigma, Greek.GAMMA))} vega=${f(totalGreek(hh.options, S0, r, sigma, Greek.VEGA))}")
    }
    line("Value after an immediate jump in S:")
    for (S in moves)
        line("S=${f(S)}: delta ${f(portfolioValue(h, S, 0.0, r, sigma))} " +
            "delta-gamma ${f(portfolioValue(hg, S, 0.0, r, sigma))} delta-vega ${f(portfolioValue(hv, S, 0.0, r, sigma))}")
    line("Value after an immediate change in sigma (S unchanged):")
    for (sg in listOf(0.15, 0.2, 0.25, 0.3))
        line("sigma=${f(sg)}: delta ${f(portfolioValue(h, S0, 0.0, r, sg))} " +
            "delta-gamma ${f(portfolioValue(hg, S0, 0.0, r, sg))} delta-vega ${f(portfolioValue(hv, S0, 0.0, r, sg))}")

    line("\n== 9.1.3 Applications: after one week S=97 and sigma=0.22 ==")
    for ((label, hh) in hedges) line("$label: ${f(portfolioValue(hh, 97.0, 1.0 / 52, r, 0.22))}")
    line("Theta: value change over one week with S and sigma unchanged")
    for ((label, hh) in hedges)
        line("$label: ${f(portfolioValue(hh, S0, 1.0 / 52, r, sigma))}  (theta x dt = " +
            "${f(totalGreek(hh.options, S0, r, sigma, Greek.THETA) / 52)})")

    line("\n== 9.2.1 Value at Risk: 1 share, S(0)=100, m=0.08 sigma=0.3 T=0.5 (lognormal) ==")
    val model = Lognormal(100.0, 0.08, 0.3, 0.5)
    val stock = unhedgedHolding(1.0)
    line("E S(T)=${f(model.expectedStock())}")
    for (conf in listOf(0.9, 0.95, 0.99))
        line("confidence ${f(conf)}: z=${f(normInv(1 - conf))} S quantile=${f(model.quantile(1 - conf))} " +
            "VaR=${f(valueAtRisk(stock, model, r, sigma, conf))}")
    line("Hedging with puts (priced with r=0.05 sigma=0.3), VaR at 95%:")
    for (Xp in listOf(85.0, 90.0, 95.0, 100.0)) {
        val st = putHedgedHolding("put X=${Xp.toInt()}", 1.0, Xp)
        line("X=${f(Xp)}: put price=${f(bsPrice(100.0, Xp, r, 0.3, 0.5, false))} initial value=${f(initialCost(st, 100.0, r, 0.3, 0.5))} " +
            "VaR=${f(valueAtRisk(st, model, r, 0.3, 0.95))} E V(T)=${f(st.expectedValue(model, r))} " +
            "expected return=${f(expectedReturn(st, model, r, 0.3))}")
    }
    val half = putHedgedHolding("half hedged X=95", 1.0, 95.0, 0.5)
    line("Half hedged X=95: VaR=${f(valueAtRisk(half, model, r, 0.3, 0.95))} " +
        "expected return=${f(expectedReturn(half, model, r, 0.3))}")

    line("\n== 9.2.2 Case study: 10000 shares to be sold in 6 months, VaR(95%) target 100000 ==")
    val big = unhedgedHolding(10000.0)
    line("Unhedged: VaR=${f(valueAtRisk(big, model, r, 0.3, 0.95))} expected return=${f(expectedReturn(big, model, r, 0.3))}")
    var chosen: Strategy? = null
    for (Xp in listOf(80.0, 85.0, 90.0, 95.0, 100.0, 105.0)) {
        val st = putHedgedHolding("put X=${Xp.toInt()}", 10000.0, Xp)
        val v = valueAtRisk(st, model, r, 0.3, 0.95)
        val cost = 10000.0 * bsPrice(100.0, Xp, r, 0.3, 0.5, false)
        line("puts X=${f(Xp)}: cost=${f(cost)} VaR=${f(v)} expected return=${f(expectedReturn(st, model, r, 0.3))}")
        if (chosen == null && v <= 100000.0) chosen = st
    }
    line("Cheapest hedge meeting the target: ${chosen!!.name}")

    line("\n== 9.3.1 Speculating: capital 10000, S(0)=100 m=0.10 sigma=0.25 T=0.5, r=0.05 ==")
    val spec = Lognormal(100.0, 0.10, 0.25, 0.5)
    run {
        val sS = allInStock(10000.0, 100.0)
        val c100 = allInCalls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.5)
        val c110 = allInCalls("calls X=110", 10000.0, 100.0, 110.0, r, 0.25, 0.5)
        val halfMix = mix("half stock, half calls X=100", sS, 0.5, c100, 0.5)
        val strategies = listOf(sS, c100, c110, halfMix)
        line("C(X=100)=${f(bsPrice(100.0, 100.0, r, 0.25, 0.5, true))} C(X=110)=${f(bsPrice(100.0, 110.0, r, 0.25, 0.5, true))}")
        for (st in strategies) line("${st.name}: shares=${f(st.shares)} options=${fl(legQtys(st))}")
        val scen = listOf(80.0, 90.0, 100.0, 110.0, 120.0, 130.0)
        line("Terminal value by scenario S(T)=" + fl(scen) + ":")
        for (st in strategies) line("${st.name}: ${fl(scen.map { st.terminalValue(it, r, 0.5) })}")
        line("Expected return, probability of loss, breakeven S(T), VaR at 95%:")
        for (st in strategies)
            line("${st.name}: E ret=${f(expectedReturn(st, spec, r, 0.25))} P(loss)=${f(probLoss(st, spec, r, 0.25))} " +
                "breakeven=${f(breakeven(st, spec, r, 0.25))} VaR=${f(valueAtRisk(st, spec, r, 0.25, 0.95))}")
    }

    line("\n== 9.3.2 Case study: expecting a rise to about 115 in 3 months (m=0.4 sigma=0.25 T=0.25) ==")
    val bull = Lognormal(100.0, 0.4, 0.25, 0.25)
    val sS = allInStock(10000.0, 100.0)
    val c100 = allInCalls("calls X=100", 10000.0, 100.0, 100.0, r, 0.25, 0.25)
    val bs = bullSpread("bull spread 100/115", 10000.0, 100.0, 100.0, 115.0, r, 0.25, 0.25)
    line("E S(T)=${f(bull.expectedStock())} P(S(T)>115)=${f(1 - bull.cdf(115.0))}")
    for (st in listOf(sS, c100, bs))
        line("${st.name}: options=${fl(legQtys(st))} E ret=${f(expectedReturn(st, bull, r, 0.25))} " +
            "P(loss)=${f(probLoss(st, bull, r, 0.25))} VaR=${f(valueAtRisk(st, bull, r, 0.25, 0.95))} " +
            "value at S(T)=115: ${f(st.terminalValue(115.0, r, 0.25))}")
    line("If instead the stock stays flat (S(T)=100): " + listOf(sS, c100, bs).joinToString(" ") {
        "${it.name}=${f(it.terminalValue(100.0, r, 0.25))}"
    })
}
