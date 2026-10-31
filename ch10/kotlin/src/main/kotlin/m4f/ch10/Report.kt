package m4f.ch10

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.exp

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

// Initial flat yield and amount invested in Sections 10.1.3 and 10.1.4.
private const val Y0 = 0.06
private const val V0 = 100000.0

/** Section 10.1.1. */
private fun StringBuilder.singleBondsSection() {
    line(
        "\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 years, " +
            "y=0.06 continuous ==",
    )
    val bond = couponBond(100.0, 5.0, 4)
    line("Price at y=0.06: ${fmt(price(bond, Y0))}")
    line(
        "Yield jumps right after purchase; value of the investment at horizon t " +
            "(coupons reinvested):",
    )
    for (y in listOf(0.05, 0.06, 0.07)) {
        val values =
            (1..4).joinToString(" ") { t ->
                "t=$t:${fmt(investmentValue(bond, y, t.toDouble()))}"
            }
        line("y=${fmt(y)}: $values")
    }
    line("Annualised log return over [0,t]:")
    for (y in listOf(0.05, 0.06, 0.07)) {
        val returns =
            (1..4).joinToString(" ") { t -> "t=$t:${fmt(logReturn(bond, Y0, y, t.toDouble()))}" }
        line("y=${fmt(y)}: $returns")
    }
}

/** Section 10.1.2. */
private fun StringBuilder.durationSection() {
    line("\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P ==")
    val bond = couponBond(100.0, 5.0, 4)
    val d = duration(bond, Y0)
    val zeroD = duration(zeroCoupon(100.0, 4.0), Y0)
    line("Coupon bond: D=${fmt(d)} years; zero-coupon 4y: D=${fmt(zeroD)}")
    val h = 1e-4
    val slope = (price(bond, Y0 + h) - price(bond, Y0 - h)) / (2 * h)
    line("dP/dy by finite difference: ${fmt(slope)}, -D P = ${fmt(-d * price(bond, Y0))}")
    line(
        "Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first order): " +
            fmtList(listOf(0.05, 0.06, 0.07).map { investmentValue(bond, it, d) }),
    )
    val examples =
        listOf(
            "5y 4%" to couponBond(100.0, 4.0, 5),
            "10y 8%" to couponBond(100.0, 8.0, 10),
            "10y zero" to zeroCoupon(100.0, 10.0),
        )
    for ((name, b) in examples) {
        line(
            "$name: price=${fmt(price(b, Y0))} D=${fmt(duration(b, Y0))} at y=0.06; " +
                "D at y=0.08: ${fmt(duration(b, 0.08))}",
        )
    }
}

/** Section 10.1.3. */
private fun StringBuilder.portfoliosSection(bondA: Bond, bondB: Bond) {
    line("\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 years, y=0.06 ==")
    val dA = duration(bondA, Y0)
    val dB = duration(bondB, Y0)
    val (wA, wB) = immunisingWeights(dA, dB, 5.0)
    line(
        "D_A=${fmt(dA)} D_B=${fmt(dB)}: weights w_A=${fmt(wA)} w_B=${fmt(wB)} " +
            "give D=${fmt(wA * dA + wB * dB)}",
    )
    val p = immunise(V0, bondA, bondB, Y0, 0.0, 5.0)
    line("Invest ${fmt(V0)}: ${fmt(p.q1)} of A and ${fmt(p.q2)} of B")
    val target = V0 * exp(Y0 * 5.0)
    line("Target at t=5 if y stays 0.06: ${fmt(target)}")
    line("Value at t=5 after an immediate jump of y (coupons reinvested at the new yield):")
    for (y in listOf(0.04, 0.05, 0.06, 0.07, 0.08)) {
        val v = p.q1 * investmentValue(bondA, y, 5.0) + p.q2 * investmentValue(bondB, y, 5.0)
        val bAlone = V0 / price(bondB, Y0) * investmentValue(bondB, y, 5.0)
        val aAlone = V0 / price(bondA, Y0) * investmentValue(bondA, y, 5.0)
        line(
            "y=${fmt(y)}: portfolio ${fmt(v)} (shortfall ${fmt(target - v)}), " +
                "B alone ${fmt(bAlone)}, A alone ${fmt(aAlone)}",
        )
    }
}

/** Section 10.1.4. */
private fun StringBuilder.dynamicHedgingSection(bondA: Bond, bondB: Bond) {
    line("\n== 10.1.4 Dynamic hedging: re-immunise every half year along a yield path ==")
    val path =
        listOf(
            0.0 to 0.06,
            0.5 to 0.065,
            1.0 to 0.07,
            1.5 to 0.06,
            2.0 to 0.05,
            2.5 to 0.055,
            3.0 to 0.065,
            3.5 to 0.07,
            4.0 to 0.075,
            4.5 to 0.07,
            5.0 to 0.065,
        )
    for (st in dynamicHedge(V0, bondA, bondB, 5.0, path)) {
        line(
            "t=${fmt(st.t)} y=${fmt(st.y)}: value=${fmt(st.value)} " +
                "q_A=${fmt(st.q1)} q_B=${fmt(st.q2)} D=${fmt(st.duration)}",
        )
    }
    val fixed = dynamicHedge(V0, bondA, bondB, 5.0, path, rebalance = false)
    val rebalanced = dynamicHedge(V0, bondA, bondB, 5.0, path)
    line(
        "At t=5: rebalanced ${fmt(rebalanced.last().value)}, " +
            "not rebalanced ${fmt(fixed.last().value)}, target ${fmt(V0 * exp(Y0 * 5.0))}",
    )
}

/** Sections 10.2.1 and 10.2.2. */
private fun StringBuilder.termStructureSection(maturities: List<Double>, prices: List<Double>) {
    line("\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==")
    val forwards = forwardRates(maturities, prices)
    for (i in maturities.indices) {
        val y = yieldRate(prices[i], maturities[i])
        line(
            "T=${fmt(maturities[i])}: B=${fmt(prices[i])} y(0,T)=${fmt(y)} " +
                "f(0,T-0.5,T)=${fmt(forwards[i])}",
        )
    }
    val rebuilt = pricesFromForwardRates(maturities, forwards)
    line("Prices rebuilt from the forward rates: ${fmtList(rebuilt)}")
    var avg = 0.0
    for (i in 2 until 6) avg += forwards[i]
    val f13 = forwardRate(prices[1], 1.0, prices[5], 3.0)
    line("f(0,1,3)=${fmt(f13)} = average of the half-year forwards ${fmt(avg / 4)}")
    val implied =
        (2 until maturities.size).joinToString(" ") {
            "T=${fmt(maturities[it])}:${fmt(impliedPrice(prices[1], prices[it]))}"
        }
    line("Implied B(1,T) = B(0,T)/B(0,1): $implied")
    val f12 = forwardRate(prices[1], 1.0, prices[3], 2.0)
    line(
        "FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)=${fmt(f12)}: " +
            "buy 1000 one-year bonds (cost ${fmt(1000.0 * prices[1])}), " +
            "sell ${fmt(1000.0 * prices[1] / prices[3])} two-year bonds; " +
            "repay ${fmt(1000.0 * exp(f12))}",
    )
    val flowTimes = listOf(1.0, 2.0, 3.0)
    val flowAmounts = listOf(6.0, 6.0, 106.0)
    val bondPrice = couponBondPrice(flowTimes, flowAmounts, maturities, prices)
    val ytm = yieldToMaturity(bondPrice, flowTimes, flowAmounts)
    line(
        "3-year 6% coupon bond under the term structure: price ${fmt(bondPrice)}, " +
            "yield to maturity ${fmt(ytm)} " +
            "(between y(0,1)=${fmt(yieldRate(prices[1], 1.0))} " +
            "and y(0,3)=${fmt(yieldRate(prices[5], 3.0))})",
    )

    line("\n== 10.2.2 Money market account: rolling over half-year bonds at the forward rates ==")
    val account = moneyMarketAccount(maturities, prices)
    for (i in maturities.indices) {
        line("A(${fmt(maturities[i])})=${fmt(account[i])} = 1/B(0,T)=${fmt(1 / prices[i])}")
    }
    var sumF = 0.0
    for (fr in forwards) sumF += fr * 0.5
    line(
        "Short rates realised = forward rates: ${fmtList(forwards)}; " +
            "A(3)=exp(sum f dt)=${fmt(exp(sumF))}",
    )
    line(
        "1 unit at t=0 in the 3-year bond: ${fmt(1 / prices[5])} at t=3, " +
            "same as the money market account ${fmt(account[5])}",
    )
}

/** The full chapter 10 report. */
fun report(): String =
    buildString {
        line("Chapter 10: Variable Interest Rates (illustrative numbers, book formulas)")
        singleBondsSection()
        durationSection()
        val bondA = couponBond(100.0, 4.0, 5)
        val bondB = couponBond(100.0, 8.0, 10)
        portfoliosSection(bondA, bondB)
        dynamicHedgingSection(bondA, bondB)
        termStructureSection(
            listOf(0.5, 1.0, 1.5, 2.0, 2.5, 3.0),
            listOf(0.978, 0.953, 0.926, 0.898, 0.870, 0.842),
        )
    }
