package m4f.ch09

import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.max
import kotlin.math.sqrt
import kotlin.test.Test
import kotlin.test.assertTrue

class Ch09Test {
    private fun near(a: Double, b: Double, eps: Double = 1e-7) = assertTrue(abs(a - b) < eps, "$a != $b")
    private val S = 100.0
    private val X = 100.0
    private val r = 0.05
    private val sigma = 0.2
    private val T = 0.5

    @Test fun normal() {
        near(normCdf(0.0), 0.5)
        near(normCdf(1.959963984540054), 0.975, 1e-9)
        near(normInv(0.975), 1.959963984540054, 1e-9)
        near(normPdf(0.0), 1 / sqrt(2 * PI), 1e-12)
    }

    @Test fun greeksByFiniteDifferences() {
        val h = 1e-4
        for (isCall in listOf(true, false)) {
            val g = bsGreeks(S, X, r, sigma, T, isCall)
            fun p(s: Double = S, x: Double = X, rr: Double = r, sg: Double = sigma, t: Double = T) = bsPrice(s, x, rr, sg, t, isCall)
            near(g.price, p())
            near(g.delta, (p(s = S + h) - p(s = S - h)) / (2 * h), 1e-6)
            near(g.gamma, (p(s = S + h) - 2 * p() + p(s = S - h)) / (h * h), 1e-4)
            near(g.theta, -(p(t = T + h) - p(t = T - h)) / (2 * h), 1e-5)
            near(g.vega, (p(sg = sigma + h) - p(sg = sigma - h)) / (2 * h), 1e-5)
            near(g.rho, (p(rr = r + h) - p(rr = r - h)) / (2 * h), 1e-5)
        }
    }

    @Test fun hedgesAreNeutralAndSelfFinancing() {
        val written = listOf(Position(-1000.0, X, T, true))
        val extra = Position(0.0, 110.0, T, true)
        val hd = deltaHedge(written, S, r, sigma)
        val hg = deltaGreekHedge(written, extra, S, r, sigma, Greek.GAMMA)
        val hv = deltaGreekHedge(written, extra, S, r, sigma, Greek.VEGA)
        for (hh in listOf(hd, hg, hv)) {
            near(portfolioValue(hh, S, 0.0, r, sigma), 0.0, 1e-8)
            near(totalGreek(hh.options, S, r, sigma, Greek.DELTA) + hh.shares, 0.0, 1e-9)
        }
        near(totalGreek(hg.options, S, r, sigma, Greek.GAMMA), 0.0, 1e-9)
        near(totalGreek(hv.options, S, r, sigma, Greek.VEGA), 0.0, 1e-6)
        assertTrue(abs(portfolioValue(hd, S + 1, 0.0, r, sigma) - totalGreek(written, S, r, sigma, Greek.GAMMA) / 2) < 0.5)
    }

    @Test fun rebalancePathIsSelfFinancing() {
        val written = listOf(Position(-1000.0, X, T, true))
        val steps = rebalancePath(written, listOf(100.0, 103.0, 101.0), 1.0 / 52, r, sigma)
        assertTrue(steps.size == 3)
        near(steps[0].value, 0.0, 1e-8)
        near(steps[1].money, steps[0].money * exp(r / 52) - (steps[1].shares - steps[0].shares) * steps[1].S, 1e-6)
    }

    @Test fun lognormal() {
        val m = Lognormal(100.0, 0.08, 0.3, 0.5)
        near(m.cdf(m.quantile(0.3)), 0.3, 1e-9)
        near(m.expectedCallPayoff(90.0) - m.expectedPutPayoff(90.0), m.expectedStock() - 90.0, 1e-9)
        val n = 20000
        val lo = -8.0
        val hi = 8.0
        var tot = 0.0
        for (i in 0 until n) {
            val z = lo + (i + 0.5) * (hi - lo) / n
            val s = 100.0 * exp(0.08 * 0.5 + 0.3 * sqrt(0.5) * z)
            tot += max(s - 90.0, 0.0) * normPdf(z) * (hi - lo) / n
        }
        near(tot, m.expectedCallPayoff(90.0), 1e-6)
    }

    @Test fun varAndStrategies() {
        val m = Lognormal(100.0, 0.08, 0.3, 0.5)
        val stock = unhedgedHolding(1.0)
        near(valueAtRisk(stock, m, r, 0.3, 0.95), 100.0 - m.quantile(0.05))
        val hedged = putHedgedHolding("p", 1.0, 95.0)
        near(valueAtRisk(hedged, m, r, 0.3, 0.95), bsPrice(100.0, 95.0, r, 0.3, 0.5, false) + 5.0)
        assertTrue(expectedReturn(hedged, m, r, 0.3) < expectedReturn(stock, m, r, 0.3))
        val V0 = 10000.0
        for (st in listOf(allInStock(V0, 100.0), allInCalls("c", V0, 100.0, 100.0, r, 0.25, 0.5),
                          bullSpread("b", V0, 100.0, 100.0, 115.0, r, 0.25, 0.5))) {
            near(initialCost(st, 100.0, r, 0.25, 0.5), V0)
            val be = breakeven(st, m, r, 0.25)
            near(st.terminalValue(be, r, 0.5), V0, 1e-6)
            near(probLoss(st, m, r, 0.25), m.cdf(be))
        }
        near(breakeven(allInCalls("c", V0, 100.0, 100.0, r, 0.25, 0.5), m, r, 0.25),
             100.0 + bsPrice(100.0, 100.0, r, 0.25, 0.5, true), 1e-6)
    }
}
