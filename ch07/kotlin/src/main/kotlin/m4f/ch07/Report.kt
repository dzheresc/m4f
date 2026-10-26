package m4f.ch07

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.abs
import kotlin.math.exp

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

private fun price(S0: Double, X: Double, T: Double, isCall: Boolean, american: Boolean = false): Double =
    crrPrice(S0, X, 0.05, 0.2, T, 200, isCall, american)

fun report(): String = buildString {
    line("Chapter 7: Options: General Properties (illustrative numbers, book formulas)")

    line("\n== 7.1 Definitions: X=100, premiums C=10 P=5, r=0.05 T=1 ==")
    for (ST in listOf(80.0, 100.0, 120.0)) {
        val c = callPayoff(ST, 100.0)
        val p = putPayoff(ST, 100.0)
        line("S(T)=${f(ST)}: call payoff=${f(c)} profit long=${f(profit(c, 10.0, 0.05, 1.0))} " +
            "short=${f(-profit(c, 10.0, 0.05, 1.0))}; put payoff=${f(p)} profit long=${f(profit(p, 5.0, 0.05, 1.0))} " +
            "short=${f(-profit(p, 5.0, 0.05, 1.0))}")
    }

    line("\n== 7.2 Put-call parity: C - P = S(0) - X e^(-rT), S(0)=100 X=100 r=0.05 T=1 ==")
    line("S(0) - X e^(-rT) = ${f(100.0 - 100.0 * exp(-0.05))}")
    for ((C, P) in listOf(Pair(10.45, 5.57), Pair(12.0, 5.0), Pair(9.0, 6.0))) {
        val gap = parityGap(C, P, 100.0, 100.0, 0.05, 1.0)
        line("C=${f(C)} P=${f(P)}: gap=${f(gap)} arbitrage profit at T=${f(abs(gap) * exp(0.05))}")
    }
    line("Dividend 2 at t=0.5 (PV ${f(2 * exp(-0.025))}): C - P should be " +
        f(-parityGap(0.0, 0.0, 100.0, 100.0, 0.05, 1.0, 2 * exp(-0.025))))

    line("\n== 7.3.1 Bounds on European options: S(0)=100 X=100 r=0.05 T=1 ==")
    val cb = europeanCallBounds(100.0, 100.0, 0.05, 1.0)
    val pb = europeanPutBounds(100.0, 100.0, 0.05, 1.0)
    line("call in ${fl(listOf(cb.lo, cb.hi))}, put in ${fl(listOf(pb.lo, pb.hi))}")
    line("Quoted call 3.50: violation ${f(boundViolation(3.5, cb))}; quoted put 96.00: violation " +
        "${f(boundViolation(96.0, pb))}; model call ${f(price(100.0, 100.0, 1.0, true))}: " +
        "violation ${f(boundViolation(price(100.0, 100.0, 1.0, true), cb))}")

    line("\n== 7.3.2 American calls on a non-dividend paying stock (CRR, sigma=0.2, N=200) ==")
    for (S0 in listOf(90.0, 100.0, 120.0))
        line("S(0)=${f(S0)}: C_E=${f(price(S0, 100.0, 1.0, true))} C_A=${f(price(S0, 100.0, 1.0, true, true))} " +
            "exercise now=${f(callPayoff(S0, 100.0))}")

    line("\n== 7.3.3 American options: S(0) - X <= C_A - P_A <= S(0) - X e^(-rT) ==")
    for (S0 in listOf(90.0, 100.0, 120.0)) {
        val PE = price(S0, 100.0, 1.0, false)
        val PA = price(S0, 100.0, 1.0, false, true)
        val CA = price(S0, 100.0, 1.0, true, true)
        val b = americanParityBounds(S0, 100.0, 0.05, 1.0)
        line("S(0)=${f(S0)}: P_E=${f(PE)} P_A=${f(PA)} C_A-P_A=${f(CA - PA)} in ${fl(listOf(b.lo, b.hi))}")
    }

    line("\n== 7.4.1 European options: dependence on X and S(0) ==")
    val Xs = listOf(90.0, 100.0, 110.0)
    val Cs = Xs.map { price(100.0, it, 1.0, true) }
    line("X=${fl(Xs)}: C=${fl(Cs)} P=${fl(Xs.map { price(100.0, it, 1.0, false) })}")
    line("C(90)-C(100)=${f(Cs[0] - Cs[1])} <= e^(-rT) x 10 = ${f(10 * exp(-0.05))}; " +
        "butterfly gap=${f(butterflyGap(Cs[0], Cs[1], Cs[2], 90.0, 100.0, 110.0))} (<= 0)")
    line("Quotes C=15, 10, 2: butterfly gap=${f(butterflyGap(15.0, 10.0, 2.0, 90.0, 100.0, 110.0))} " +
        "-> write 2 x C(100), buy C(90) and C(110)")
    val Ss = listOf(90.0, 100.0, 110.0)
    line("S(0)=${fl(Ss)}: C=${fl(Ss.map { price(it, 100.0, 1.0, true) })}")

    line("\n== 7.4.2 American options: dependence on T ==")
    val Ts = listOf(0.25, 0.5, 1.0, 2.0)
    line("T=${fl(Ts)}: P_A=${fl(Ts.map { price(100.0, 100.0, it, false, true) })}")

    line("\n== 7.5 Time value of options: price - intrinsic value, X=100 T=1 ==")
    for (S0 in listOf(80.0, 100.0, 120.0)) {
        val C = price(S0, 100.0, 1.0, true)
        val P = price(S0, 100.0, 1.0, false)
        line("S(0)=${f(S0)}: call ${f(C)} time value ${f(C - callPayoff(S0, 100.0))}; " +
            "put ${f(P)} time value ${f(P - putPayoff(S0, 100.0))}")
    }
}
