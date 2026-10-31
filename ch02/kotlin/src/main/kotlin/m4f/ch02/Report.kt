package m4f.ch02

import java.math.BigDecimal
import java.math.RoundingMode
import kotlin.math.ln

// Round-half-even on the exact binary value, like C++ std::format and Python, so the report is
// byte-identical across the three implementations. Adding 0.0 turns -0.0 into 0.0.
private fun fmt(v: Double): String =
    BigDecimal(v + 0.0).setScale(4, RoundingMode.HALF_EVEN).toPlainString()

private fun StringBuilder.line(s: String) {
    append(s).append('\n')
}

private fun StringBuilder.simpleInterest() {
    line("\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==")
    for (t in listOf(0.5, 1.0, 2.0)) {
        line("P=2000 r=0.08 t=${fmt(t)}: V(t)=${fmt(simpleValue(2000.0, 0.08, t))}")
    }
    line(
        "Present value of 1000 due at t=0.5, r=0.10: " +
            fmt(simplePresentValue(1000.0, 0.10, 0.5)),
    )
    line("Rate turning 1000 into 1100 in 2 years: ${fmt(simpleRate(1000.0, 1100.0, 2.0))}")
}

private fun StringBuilder.periodicCompounding() {
    line("\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P ==")
    for (m in listOf(1, 2, 4, 12, 365)) {
        line(
            "P=1000 r=0.12 m=$m: V(1)=${fmt(periodicValue(1000.0, 0.12, m, 1.0))} " +
                "effective rate=${fmt(effectiveRate(0.12, m))}",
        )
    }
    line(
        "Present value of 1000 due in 3 years, r=0.10 annual: " +
            fmt(periodicPresentValue(1000.0, 0.10, 1, 3.0)),
    )
}

private fun StringBuilder.streamsOfPayments() {
    line("\n== 2.1.3 Streams of payments ==")
    line(
        "Annuity of 10 x 1000 at 10%: PV=${fmt(annuityPresentValue(1000.0, 0.10, 10))} " +
            "FV=${fmt(annuityFutureValue(1000.0, 0.10, 10))}",
    )
    line("Perpetuity of 100 a year at 5%: PV=${fmt(perpetuityPresentValue(100.0, 0.05))}")
    val installment = loanInstallment(100000.0, 0.06 / 12, 300)
    val balance = annuityPresentValue(installment, 0.06 / 12, 180)
    line(
        "Loan 100000 over 25 years, monthly at 6%: installment=${fmt(installment)} " +
            "balance after 10 years=${fmt(balance)}",
    )
}

private fun StringBuilder.continuousCompounding() {
    line("\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==")
    line(
        "P=1000 r=0.12: V(1)=${fmt(continuousValue(1000.0, 0.12, 1.0))} " +
            "effective rate=${fmt(continuousEffectiveRate(0.12))}",
    )
    line(
        "Present value of 1000 due in 3 years, r=0.10 continuous: " +
            fmt(continuousPresentValue(1000.0, 0.10, 3.0)),
    )
}

private fun StringBuilder.comparingMethods() {
    line("\n== 2.1.5 Comparing compounding methods (r=0.10, one year) ==")
    line(
        "Growth factor: simple ${fmt(simpleValue(1.0, 0.10, 1.0))}, " +
            "m=1 ${fmt(periodicValue(1.0, 0.10, 1, 1.0))}, " +
            "m=2 ${fmt(periodicValue(1.0, 0.10, 2, 1.0))}, " +
            "m=12 ${fmt(periodicValue(1.0, 0.10, 12, 1.0))}, " +
            "continuous ${fmt(continuousValue(1.0, 0.10, 1.0))}",
    )
    line("12% monthly = ${fmt(continuousFromPeriodic(0.12, 12))} continuous")
    line(
        "10% continuous = ${fmt(periodicFromContinuous(0.10, 1))} annual = " +
            "${fmt(periodicFromContinuous(0.10, 12))} monthly",
    )
    line(
        "12% quarterly = ${fmt(periodicFromPeriodic(0.12, 4, 12))} monthly = " +
            "${fmt(periodicFromPeriodic(0.12, 4, 1))} annual",
    )
}

private fun StringBuilder.zeroCouponBonds() {
    line("\n== 2.2.1 Zero-coupon bonds ==")
    line(
        "B(0,1)=0.92 implies: annual ${fmt(impliedRate(0.92, 1.0, 1))}, " +
            "semi-annual ${fmt(impliedRate(0.92, 1.0, 2))}, " +
            "continuous ${fmt(impliedRate(0.92, 1.0))}",
    )
    line(
        "Face 100, T=2 at 10%: annual ${fmt(100.0 * zeroCouponPrice(0.10, 2.0, 1))}, " +
            "continuous ${fmt(100.0 * zeroCouponPrice(0.10, 2.0))}",
    )
}

private fun StringBuilder.couponBonds() {
    line("\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years ==")
    for (y in listOf(0.12, 0.10, 0.08)) {
        line("yield ${fmt(y)}: price ${fmt(couponBondPrice(100.0, 10.0, 3, y))}")
    }
    line("price 95.0000: yield to maturity ${fmt(yieldToMaturity(95.0, 100.0, 10.0, 3))}")
}

private fun StringBuilder.moneyMarketAccount() {
    line("\n== 2.2.3 Money market account ==")
    line("A(0)=100 at 8% continuous: A(1)=${fmt(moneyMarketValue(100.0, 0.08, 1.0))}")
    val a1 = moneyMarketRollover(100.0, listOf(0.98, 0.985, 0.97, 0.99))
    line(
        "Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): " +
            "A(1)=${fmt(a1)} continuous rate ${fmt(ln(a1 / 100.0))}",
    )
}

/** The chapter's worked examples as text. */
fun report(): String = buildString {
    line("Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)")
    simpleInterest()
    periodicCompounding()
    streamsOfPayments()
    continuousCompounding()
    comparingMethods()
    zeroCouponBonds()
    couponBonds()
    moneyMarketAccount()
}
