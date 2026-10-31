package m4f.ch07

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.abs
import kotlin.math.exp

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

/** "[a, b, c]" with each entry formatted by [fmt]. */
private fun fmtList(values: List<Double>): String = values.joinToString(", ", "[", "]") { fmt(it) }

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

/** The model price used throughout: r = 0.05, sigma = 0.2, 200 steps. */
private fun price(
    s0: Double,
    strike: Double,
    maturity: Double,
    isCall: Boolean,
    american: Boolean = false,
): Double = crrPrice(s0, strike, 0.05, 0.2, maturity, 200, isCall, american)

/** Section 7.1. */
private fun StringBuilder.definitionsSection() {
    line("\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==")
    for (sT in listOf(80.0, 100.0, 120.0)) {
        val call = callPayoff(sT, 100.0)
        val put = putPayoff(sT, 100.0)
        line(
            "S(T)=${fmt(sT)}: call payoff=${fmt(call)} " +
                "profit long=${fmt(profit(call, 10.0, 0.05, 1.0))} " +
                "short=${fmt(-profit(call, 10.0, 0.05, 1.0))}; put payoff=${fmt(put)} " +
                "profit long=${fmt(profit(put, 5.0, 0.05, 1.0))} " +
                "short=${fmt(-profit(put, 5.0, 0.05, 1.0))}",
        )
    }
}

/** Section 7.2. */
private fun StringBuilder.paritySection() {
    line("\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 r=0.05 T=1 ==")
    line("S(0) - X e^(-rT) = ${fmt(100.0 - 100.0 * exp(-0.05))}")
    for ((call, put) in listOf(Pair(10.45, 5.57), Pair(12.0, 5.0), Pair(9.0, 6.0))) {
        val gap = parityGap(call, put, 100.0, 100.0, 0.05, 1.0)
        line(
            "C=${fmt(call)} P=${fmt(put)}: gap=${fmt(gap)} " +
                "arbitrage profit at T=${fmt(abs(gap) * exp(0.05))}",
        )
    }
    val pvDiv = 2 * exp(-0.025)
    val expected = -parityGap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, pvDiv)
    line("Dividend 2 at t=0.5 (PV ${fmt(pvDiv)}): C - P should be ${fmt(expected)}")
}

/** Section 7.3.1. */
private fun StringBuilder.europeanBoundsSection() {
    line("\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 ==")
    val callBounds = europeanCallBounds(100.0, 100.0, 0.05, 1.0)
    val putBounds = europeanPutBounds(100.0, 100.0, 0.05, 1.0)
    line(
        "call in ${fmtList(listOf(callBounds.lo, callBounds.hi))}, " +
            "put in ${fmtList(listOf(putBounds.lo, putBounds.hi))}",
    )
    val modelCall = price(100.0, 100.0, 1.0, true)
    line(
        "Quoted call 3.50: violation ${fmt(boundViolation(3.5, callBounds))}; " +
            "quoted put 96.00: violation ${fmt(boundViolation(96.0, putBounds))}; " +
            "model call ${fmt(modelCall)}: violation ${fmt(boundViolation(modelCall, callBounds))}",
    )
}

/** Sections 7.3.2 and 7.3.3. */
private fun StringBuilder.americanSection() {
    line("\n== 7.3.2 American calls on a non-dividend paying stock (CRR, sigma=0.2, N=200) ==")
    for (s0 in listOf(90.0, 100.0, 120.0)) {
        line(
            "S(0)=${fmt(s0)}: C_E=${fmt(price(s0, 100.0, 1.0, true))} " +
                "C_A=${fmt(price(s0, 100.0, 1.0, true, true))} " +
                "exercise now=${fmt(callPayoff(s0, 100.0))}",
        )
    }

    line("\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) ==")
    for (s0 in listOf(90.0, 100.0, 120.0)) {
        val putE = price(s0, 100.0, 1.0, false)
        val putA = price(s0, 100.0, 1.0, false, true)
        val callA = price(s0, 100.0, 1.0, true, true)
        val b = americanParityBounds(s0, 100.0, 0.05, 1.0)
        line(
            "S(0)=${fmt(s0)}: P_E=${fmt(putE)} P_A=${fmt(putA)} C_A-P_A=${fmt(callA - putA)} " +
                "in ${fmtList(listOf(b.lo, b.hi))}",
        )
    }
}

/** Sections 7.4.1 and 7.4.2. */
private fun StringBuilder.variablesSection() {
    line("\n== 7.4.1 European options: dependence on X and S(0) ==")
    val strikes = listOf(90.0, 100.0, 110.0)
    val calls = strikes.map { price(100.0, it, 1.0, true) }
    val puts = strikes.map { price(100.0, it, 1.0, false) }
    line("X=${fmtList(strikes)}: C=${fmtList(calls)} P=${fmtList(puts)}")
    val gap = butterflyGap(calls[0], calls[1], calls[2], 90.0, 100.0, 110.0)
    line(
        "C(90)-C(100)=${fmt(calls[0] - calls[1])} <= e^(-rT) x 10 = ${fmt(10 * exp(-0.05))}; " +
            "butterfly gap=${fmt(gap)} (<= 0)",
    )
    val quotedGap = butterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0)
    line(
        "Quotes C=15, 10, 2: butterfly gap=${fmt(quotedGap)} " +
            "-> write 2 x C(100), buy C(90) and C(110)",
    )
    val spots = listOf(90.0, 100.0, 110.0)
    line("S(0)=${fmtList(spots)}: C=${fmtList(spots.map { price(it, 100.0, 1.0, true) })}")

    line("\n== 7.4.2 American options: dependence on T ==")
    val maturities = listOf(0.25, 0.5, 1.0, 2.0)
    val americanPuts = maturities.map { price(100.0, 100.0, it, false, true) }
    line("T=${fmtList(maturities)}: P_A=${fmtList(americanPuts)}")
}

/** Section 7.5. */
private fun StringBuilder.timeValueSection() {
    line("\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 ==")
    for (s0 in listOf(80.0, 100.0, 120.0)) {
        val call = price(s0, 100.0, 1.0, true)
        val put = price(s0, 100.0, 1.0, false)
        line(
            "S(0)=${fmt(s0)}: call ${fmt(call)} time value ${fmt(call - callPayoff(s0, 100.0))}; " +
                "put ${fmt(put)} time value ${fmt(put - putPayoff(s0, 100.0))}",
        )
    }
}

/** The full chapter 7 report. */
fun report(): String = buildString {
    line("Chapter 7: Options: General Properties (illustrative numbers, book formulas)")
    definitionsSection()
    paritySection()
    europeanBoundsSection()
    americanSection()
    variablesSection()
    timeValueSection()
}
