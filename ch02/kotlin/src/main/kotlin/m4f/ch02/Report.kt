package m4f.ch02

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.ln

// Round-half-even on the exact binary value, like C++ std::format and Python,
// so the report is byte-identical across the three implementations.
private fun f(v: Double): String = BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun StringBuilder.line(s: String) = append(s).append('\n')

fun report(): String = buildString {
    line("Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)")

    line("\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==")
    for (t in listOf(0.5, 1.0, 2.0))
        line("P=2000 r=0.08 t=${f(t)}: V(t)=${f(simpleValue(2000.0, 0.08, t))}")
    line("Present value of 1000 due at t=0.5, r=0.10: ${f(simplePresentValue(1000.0, 0.10, 0.5))}")
    line("Rate turning 1000 into 1100 in 2 years: ${f(simpleRate(1000.0, 1100.0, 2.0))}")

    line("\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==")
    for (m in listOf(1, 2, 4, 12, 365))
        line("P=1000 r=0.12 m=$m: V(1)=${f(periodicValue(1000.0, 0.12, m, 1.0))} " +
            "effective rate=${f(effectiveRate(0.12, m))}")
    line("Present value of 1000 due in 3 years, r=0.10 annual: ${f(periodicPresentValue(1000.0, 0.10, 1, 3.0))}")

    line("\n== 2.1.3 Streams of payments ==")
    line("Annuity of 10 x 1000 at 10%: PV=${f(annuityPresentValue(1000.0, 0.10, 10))} " +
        "FV=${f(annuityFutureValue(1000.0, 0.10, 10))}")
    line("Perpetuity of 100 a year at 5%: PV=${f(perpetuityPresentValue(100.0, 0.05))}")
    val C = loanInstallment(100000.0, 0.06 / 12, 300)
    line("Loan 100000 over 25 years, monthly at 6%: installment=${f(C)} " +
        "balance after 10 years=${f(annuityPresentValue(C, 0.06 / 12, 180))}")

    line("\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==")
    line("P=1000 r=0.12: V(1)=${f(continuousValue(1000.0, 0.12, 1.0))} " +
        "effective rate=${f(continuousEffectiveRate(0.12))}")
    line("Present value of 1000 due in 3 years, r=0.10 continuous: ${f(continuousPresentValue(1000.0, 0.10, 3.0))}")

    line("\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==")
    line("Growth factor: simple ${f(simpleValue(1.0, 0.10, 1.0))}, " +
        "m=1 ${f(periodicValue(1.0, 0.10, 1, 1.0))}, m=2 ${f(periodicValue(1.0, 0.10, 2, 1.0))}, " +
        "m=12 ${f(periodicValue(1.0, 0.10, 12, 1.0))}, continuous ${f(continuousValue(1.0, 0.10, 1.0))}")
    line("12% monthly = ${f(continuousFromPeriodic(0.12, 12))} continuous")
    line("10% continuous = ${f(periodicFromContinuous(0.10, 1))} annual = ${f(periodicFromContinuous(0.10, 12))} monthly")
    line("12% quarterly = ${f(periodicFromPeriodic(0.12, 4, 12))} monthly = ${f(periodicFromPeriodic(0.12, 4, 1))} annual")

    line("\n== 2.2.1 Zero-coupon bonds ==")
    line("B(0,1)=0.92 implies: annual ${f(impliedRate(0.92, 1.0, 1))}, " +
        "semi-annual ${f(impliedRate(0.92, 1.0, 2))}, continuous ${f(impliedRate(0.92, 1.0))}")
    line("Face 100, T=2 at 10%: annual ${f(100.0 * zeroCouponPrice(0.10, 2.0, 1))}, " +
        "continuous ${f(100.0 * zeroCouponPrice(0.10, 2.0))}")

    line("\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==")
    for (y in listOf(0.12, 0.10, 0.08))
        line("yield ${f(y)}: price ${f(couponBondPrice(100.0, 10.0, 3, y))}")
    line("price 95.0000: yield to maturity ${f(yieldToMaturity(95.0, 100.0, 10.0, 3))}")

    line("\n== 2.2.3 Money market account ==")
    line("A(0)=100 at 8% continuous: A(1)=${f(moneyMarketValue(100.0, 0.08, 1.0))}")
    val A1 = moneyMarketRollover(100.0, listOf(0.98, 0.985, 0.97, 0.99))
    line("Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): A(1)=${f(A1)} " +
        "continuous rate ${f(ln(A1 / 100.0))}")
}
