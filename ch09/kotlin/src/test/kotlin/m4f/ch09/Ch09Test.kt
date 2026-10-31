package m4f.ch09

import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.math.sqrt
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class Ch09Test {
    private val s = 100.0
    private val strike = 100.0
    private val rate = 0.05
    private val sigma = 0.2
    private val maturity = 0.5

    private fun assertNear(expected: Double, actual: Double, eps: Double = 1e-7) =
        assertTrue(abs(expected - actual) < eps, "expected $expected, got $actual")

    @Test
    fun normal() {
        assertNear(0.5, normCdf(0.0))
        assertNear(0.975, normCdf(1.959963984540054), 1e-9)
        assertNear(1.959963984540054, normInv(0.975), 1e-9)
        assertNear(1 / sqrt(2 * PI), normPdf(0.0), 1e-12)
    }

    /**
     * normInv against the bisection it replaced, across the center, both tails and the region
     * boundaries of the rational approximation. Near p = 1 the inverse is only defined to within
     * one ulp of p divided by the density, so the tolerance includes that term.
     */
    @Test
    fun normInvAgainstBisection() {
        fun bisect(p: Double): Double {
            var lo = -40.0
            var hi = 40.0
            repeat(200) {
                val mid = (lo + hi) / 2
                if (normCdf(mid) < p) lo = mid else hi = mid
            }
            return (lo + hi) / 2
        }
        val probs = listOf(
            1e-100, 1e-20, 1e-10, 1e-4, 0.001, 0.01, 0.02425, 0.0243, 0.1, 0.3, 0.5, 0.7, 0.95,
            0.975, 0.97575, 0.99, 0.999, 0.999999,
        )
        for (p in probs) {
            val x = normInv(p)
            val ref = bisect(p)
            assertTrue(abs(x - ref) <= 1e-13 + 4e-16 / normPdf(ref), "p = $p")
            assertTrue(abs(normCdf(x) - p) <= 1e-14 * max(p, 1e-2), "p = $p")
        }
        assertEquals(-40.0, normInv(0.0))
        assertEquals(40.0, normInv(1.0))
        // Beyond normCdf's range (|x| > 37) the result stays finite and ordered.
        assertTrue(normInv(1e-300) < -37 && normInv(1e-300) > -40)
    }

    @Test
    fun greeksByFiniteDifferences() {
        val h = 1e-4
        for (isCall in listOf(true, false)) {
            val g = bsGreeks(s, strike, rate, sigma, maturity, isCall)
            fun price(
                s: Double = this.s,
                r: Double = rate,
                sg: Double = sigma,
                t: Double = maturity,
            ) = bsPrice(s, strike, r, sg, t, isCall)
            assertNear(price(), g.price)
            assertNear((price(s = s + h) - price(s = s - h)) / (2 * h), g.delta, 1e-6)
            assertNear((price(s = s + h) - 2 * price() + price(s = s - h)) / (h * h), g.gamma, 1e-4)
            val thetaDiff = -(price(t = maturity + h) - price(t = maturity - h)) / (2 * h)
            assertNear(thetaDiff, g.theta, 1e-5)
            assertNear((price(sg = sigma + h) - price(sg = sigma - h)) / (2 * h), g.vega, 1e-5)
            assertNear((price(r = rate + h) - price(r = rate - h)) / (2 * h), g.rho, 1e-5)
        }
    }

    @Test
    fun hedgesAreNeutralAndSelfFinancing() {
        val written = listOf(Position(-1000.0, strike, maturity, true))
        val extra = Position(0.0, 110.0, maturity, true)
        val hDelta = deltaHedge(written, s, rate, sigma)
        val hGamma = deltaGreekHedge(written, extra, s, rate, sigma, Greek.GAMMA)
        val hVega = deltaGreekHedge(written, extra, s, rate, sigma, Greek.VEGA)
        for (hedge in listOf(hDelta, hGamma, hVega)) {
            assertNear(0.0, portfolioValue(hedge, s, 0.0, rate, sigma), 1e-8)
            assertNear(
                0.0,
                totalGreek(hedge.options, s, rate, sigma, Greek.DELTA) + hedge.shares,
                1e-9,
            )
        }
        assertNear(0.0, totalGreek(hGamma.options, s, rate, sigma, Greek.GAMMA), 1e-9)
        assertNear(0.0, totalGreek(hVega.options, s, rate, sigma, Greek.VEGA), 1e-6)
        // Second order: the delta hedge error is about gamma/2 * dS^2.
        val error = portfolioValue(hDelta, s + 1, 0.0, rate, sigma)
        assertTrue(abs(error - totalGreek(written, s, rate, sigma, Greek.GAMMA) / 2) < 0.5)
    }

    @Test
    fun rebalancePathIsSelfFinancing() {
        val written = listOf(Position(-1000.0, strike, maturity, true))
        val steps = rebalancePath(written, listOf(100.0, 103.0, 101.0), 1.0 / 52, rate, sigma)
        assertEquals(3, steps.size)
        assertNear(0.0, steps[0].value, 1e-8)
        assertNear(
            steps[0].money * exp(rate / 52) - (steps[1].shares - steps[0].shares) * steps[1].s,
            steps[1].money,
            1e-6,
        )
    }

    @Test
    fun lognormal() {
        val model = Lognormal(100.0, 0.08, 0.3, 0.5)
        assertNear(0.3, model.cdf(model.quantile(0.3)), 1e-9)
        // Call and put expectations satisfy E S - X = E call - E put.
        assertNear(
            model.expectedStock() - 90.0,
            model.expectedCallPayoff(90.0) - model.expectedPutPayoff(90.0),
            1e-9,
        )
        // Expected call payoff by midpoint integration over Z.
        val n = 20000
        val lo = -8.0
        val hi = 8.0
        var total = 0.0
        for (i in 0 until n) {
            val z = lo + (i + 0.5) * (hi - lo) / n
            val st = 100.0 * exp(0.08 * 0.5 + 0.3 * sqrt(0.5) * z)
            total += max(st - 90.0, 0.0) * normPdf(z) * (hi - lo) / n
        }
        assertNear(model.expectedCallPayoff(90.0), total, 1e-6)
    }

    @Test
    fun varAndStrategies() {
        val model = Lognormal(100.0, 0.08, 0.3, 0.5)
        val stock = unhedgedHolding(1.0)
        assertNear(100.0 - model.quantile(0.05), valueAtRisk(stock, model, rate, 0.3, 0.95))
        val hedged = putHedgedHolding("p", 1.0, 95.0)
        assertNear(
            bsPrice(100.0, 95.0, rate, 0.3, 0.5, false) + 5.0,
            valueAtRisk(hedged, model, rate, 0.3, 0.95),
        )
        assertTrue(
            expectedReturn(hedged, model, rate, 0.3) < expectedReturn(stock, model, rate, 0.3),
        )
        val v0 = 10000.0
        val strategies = listOf(
            allInStock(v0, 100.0),
            allInCalls("c", v0, 100.0, 100.0, rate, 0.25, 0.5),
            bullSpread("b", v0, 100.0, 100.0, 115.0, rate, 0.25, 0.5),
        )
        for (st in strategies) {
            assertNear(v0, initialCost(st, 100.0, rate, 0.25, 0.5))
            val be = breakeven(st, model, rate, 0.25)
            assertNear(v0, st.terminalValue(be, rate, 0.5), 1e-6)
            assertNear(model.cdf(be), probLoss(st, model, rate, 0.25))
        }
        assertNear(
            100.0 + bsPrice(100.0, 100.0, rate, 0.25, 0.5, true),
            breakeven(allInCalls("c", v0, 100.0, 100.0, rate, 0.25, 0.5), model, rate, 0.25),
            1e-6,
        )
    }
}
