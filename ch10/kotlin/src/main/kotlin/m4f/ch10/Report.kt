package m4f.ch10

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.exp

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations. A tiny
// negative that rounds to zero prints as "-0.0000" there, so it does here too.
private fun f(v: Double): String {
    val s = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()
    return if (v < 0 && !s.startsWith("-")) "-$s" else s
}

private fun fl(vs: List<Double>): String = vs.joinToString(", ", "[", "]") { f(it) }

private fun StringBuilder.line(s: String) = append(s).append('\n')

fun report(): String = buildString {
    line("Chapter 10: Variable Interest Rates (illustrative numbers, book formulas)")

    line("\n== 10.1.1 Investment in single bonds: F=100, 5% annual coupon, 4 years, y=0.06 continuous ==")
    val bond = couponBond(100.0, 5.0, 4)
    val y0 = 0.06
    line("Price at y=0.06: ${f(price(bond, y0))}")
    line("Yield jumps right after purchase; value of the investment at horizon t (coupons reinvested):")
    for (y in listOf(0.05, 0.06, 0.07))
        line("y=${f(y)}: " + (1..4).joinToString(" ") { t -> "t=$t:${f(investmentValue(bond, y, t.toDouble()))}" })
    line("Annualised log return over [0,t]:")
    for (y in listOf(0.05, 0.06, 0.07))
        line("y=${f(y)}: " + (1..4).joinToString(" ") { t -> "t=$t:${f(logReturn(bond, y0, y, t.toDouble()))}" })

    line("\n== 10.1.2 Duration: D = sum t_i C_i e^(-y t_i) / P, dP/dy = -D P ==")
    val D = duration(bond, y0)
    line("Coupon bond: D=${f(D)} years; zero-coupon 4y: D=${f(duration(zeroCoupon(100.0, 4.0), y0))}")
    val h = 1e-4
    line("dP/dy by finite difference: ${f((price(bond, y0 + h) - price(bond, y0 - h)) / (2 * h))}, -D P = ${f(-D * price(bond, y0))}")
    line("Value at the horizon t=D for y=0.05, 0.06, 0.07 (immunised to first order): " +
        fl(listOf(0.05, 0.06, 0.07).map { investmentValue(bond, it, D) }))
    for ((name, b) in listOf("5y 4%" to couponBond(100.0, 4.0, 5), "10y 8%" to couponBond(100.0, 8.0, 10), "10y zero" to zeroCoupon(100.0, 10.0)))
        line("$name: price=${f(price(b, y0))} D=${f(duration(b, y0))} at y=0.06; D at y=0.08: ${f(duration(b, 0.08))}")

    line("\n== 10.1.3 Portfolios of bonds: A = 5y 4%, B = 10y 8%, horizon 5 years, y=0.06 ==")
    val A = couponBond(100.0, 4.0, 5)
    val B = couponBond(100.0, 8.0, 10)
    val DA = duration(A, y0)
    val DB = duration(B, y0)
    val (wA, wB) = immunisingWeights(DA, DB, 5.0)
    line("D_A=${f(DA)} D_B=${f(DB)}: weights w_A=${f(wA)} w_B=${f(wB)} give D=${f(wA * DA + wB * DB)}")
    val V0 = 100000.0
    val p = immunise(V0, A, B, y0, 0.0, 5.0)
    line("Invest ${f(V0)}: ${f(p.q1)} of A and ${f(p.q2)} of B")
    val target = V0 * exp(y0 * 5.0)
    line("Target at t=5 if y stays 0.06: ${f(target)}")
    line("Value at t=5 after an immediate jump of y (coupons reinvested at the new yield):")
    for (y in listOf(0.04, 0.05, 0.06, 0.07, 0.08)) {
        val v = p.q1 * investmentValue(A, y, 5.0) + p.q2 * investmentValue(B, y, 5.0)
        line("y=${f(y)}: portfolio ${f(v)} (shortfall ${f(target - v)}), B alone ${f(V0 / price(B, y0) * investmentValue(B, y, 5.0))}, " +
            "A alone ${f(V0 / price(A, y0) * investmentValue(A, y, 5.0))}")
    }

    line("\n== 10.1.4 Dynamic hedging: re-immunise every half year along a yield path ==")
    val path = listOf(0.0 to 0.06, 0.5 to 0.065, 1.0 to 0.07, 1.5 to 0.06, 2.0 to 0.05, 2.5 to 0.055, 3.0 to 0.065,
        3.5 to 0.07, 4.0 to 0.075, 4.5 to 0.07, 5.0 to 0.065)
    for (st in dynamicHedge(V0, A, B, 5.0, path))
        line("t=${f(st.t)} y=${f(st.y)}: value=${f(st.value)} q_A=${f(st.q1)} q_B=${f(st.q2)} D=${f(st.duration)}")
    val static = dynamicHedge(V0, A, B, 5.0, path, rebalance = false)
    line("At t=5: rebalanced ${f(dynamicHedge(V0, A, B, 5.0, path).last().value)}, " +
        "not rebalanced ${f(static.last().value)}, target ${f(target)}")

    line("\n== 10.2.1 Forward rates: B(0,T) for T=0.5..3 ==")
    val Ts = listOf(0.5, 1.0, 1.5, 2.0, 2.5, 3.0)
    val Bs = listOf(0.978, 0.953, 0.926, 0.898, 0.870, 0.842)
    val fw = forwardRates(Ts, Bs)
    for (i in Ts.indices)
        line("T=${f(Ts[i])}: B=${f(Bs[i])} y(0,T)=${f(yieldRate(Bs[i], Ts[i]))} f(0,T-0.5,T)=${f(fw[i])}")
    line("Prices rebuilt from the forward rates: ${fl(pricesFromForwardRates(Ts, fw))}")
    var avg = 0.0
    for (i in 2 until 6) avg += fw[i]
    line("f(0,1,3)=${f(forwardRate(Bs[1], 1.0, Bs[5], 3.0))} = average of the half-year forwards ${f(avg / 4)}")
    line("Implied B(1,T) = B(0,T)/B(0,1): " + (2 until Ts.size).joinToString(" ") { "T=${f(Ts[it])}:${f(impliedPrice(Bs[1], Bs[it]))}" })
    line("FRA: borrow 1000 at t=1, repay at t=2 at f(0,1,2)=${f(forwardRate(Bs[1], 1.0, Bs[3], 2.0))}: " +
        "buy 1000 one-year bonds (cost ${f(1000.0 * Bs[1])}), sell ${f(1000.0 * Bs[1] / Bs[3])} two-year bonds; " +
        "repay ${f(1000.0 * exp(forwardRate(Bs[1], 1.0, Bs[3], 2.0)))}")
    val ft = listOf(1.0, 2.0, 3.0)
    val fC = listOf(6.0, 6.0, 106.0)
    val P = couponBondPrice(ft, fC, Ts, Bs)
    line("3-year 6% coupon bond under the term structure: price ${f(P)}, yield to maturity ${f(yieldToMaturity(P, ft, fC))} " +
        "(between y(0,1)=${f(yieldRate(Bs[1], 1.0))} and y(0,3)=${f(yieldRate(Bs[5], 3.0))})")

    line("\n== 10.2.2 Money market account: rolling over half-year bonds at the forward rates ==")
    val As = moneyMarketAccount(Ts, Bs)
    for (i in Ts.indices) line("A(${f(Ts[i])})=${f(As[i])} = 1/B(0,T)=${f(1 / Bs[i])}")
    var sumf = 0.0
    for (fr in fw) sumf += fr * 0.5
    line("Short rates realised = forward rates: ${fl(fw)}; A(3)=exp(sum f dt)=${f(exp(sumf))}")
    line("1 unit at t=0 in the 3-year bond: ${f(1 / Bs[5])} at t=3, same as the money market account ${f(As[5])}")
}
