package m4f.ch01

import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class OneStepMarketTest {
    private val m = OneStepMarket(100.0, 110.0, 50.0, 60.0, 40.0, 0.8)
    private fun near(a: Double, b: Double) = assertTrue(abs(a - b) < 1e-9, "$a != $b")

    @Test fun returns() { near(m.r, 0.1); near(m.u, 0.2); near(m.d, -0.2); assertTrue(m.isArbitrageFree) }
    @Test fun riskNeutral() = near(m.riskNeutralProb, 0.75)
    @Test fun forward() = near(m.forwardPrice, 55.0)

    @Test fun callReplication() {
        val c = m.replicate(10.0, 0.0)
        near(c.x, 0.5); near(c.y, -2.0 / 11)
        near(m.price(10.0, 0.0), 75.0 / 11)
        near(m.price(10.0, 0.0), m.riskNeutralPrice(10.0, 0.0))
    }

    @Test fun putCallParity() = near(m.price(10.0, 0.0) - m.price(0.0, 10.0), 50 - 50 / 1.1)

    @Test fun bookExamplesAndExercises() {
        val ex = OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)
        near(ex.u, 0.04); near(ex.d, -0.04); assertTrue(!ex.isArbitrageFree)
        val P = Portfolio(20.0, 10.0)
        near(ex.value0(P), 2000.0); near(ex.value1(P, State.Up), 2140.0); near(ex.value1(P, State.Down), 2060.0)
        near(ex.portfolioReturn(P, State.Up), 0.07); near(ex.portfolioReturn(P, State.Down), 0.03)

        val er = OneStepMarket(90.0, 100.0, 25.0, 30.0, 20.0, 0.5)
        val Q = Portfolio(10.0, 15.0)
        near(er.value0(Q), 1600.0); near(er.value1(Q, State.Up), 1800.0); near(er.value1(Q, State.Down), 1700.0)
        val R = er.replicate(1160.0, 1040.0)
        near(R.x, 12.0); near(R.y, 8.0); near(er.value0(R), 1020.0)
    }

    @Test fun currencyArbitrage() {
        val fx = FxMarket(listOf(
            FxQuote("A", "EUR", "USD", 1.0202, 1.0284), FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
            FxQuote("B", "EUR", "GBP", 0.6324, 0.6401), FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
        ))
        val c = fx.bestCycle("USD")
        assertTrue(c.legs.map { it.to } == listOf("GBP", "EUR", "USD"))
        near(c.growth, 1.0202 / (1.5844 * 0.6401))
    }

    @Test fun example14() {
        val m4 = OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
        near(m4.u, 0.25); near(m4.d, 0.05); assertTrue(m4.isArbitrageFree)
        near(m4.riskNeutralProb, 0.25); near(m4.forwardPrice, 110.0)
    }

    @Test fun riskAndReturn() {
        val rr = OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)
        fun ev(P: Portfolio, E: Double, sd: Double) {
            val ku = rr.portfolioReturn(P, State.Up); val kd = rr.portfolioReturn(P, State.Down)
            near(rr.expect(ku, kd), E); near(rr.stddev(ku, kd), sd)
        }
        ev(Portfolio(50.0, 60.0), 0.12, 0.08); ev(Portfolio(0.0, 100.0), 0.10, 0.0); ev(Portfolio(125.0, 0.0), 0.15, 0.20)
        val H = rr.split(10000.0, 0.5)
        near(H.x, 62.5); near(H.y, 50.0); ev(H, 0.125, 0.10)
    }

    @Test fun forwards() {
        val P = assertNotNull(forwardArbitrage(34.0, 100.0, 112.0, 38.60))
        near(P.value0(34.0, 100.0), 0.0)
        near(P.value1(30.0, 112.0, 38.60), 0.52); near(P.value1(50.0, 112.0, 38.60), 0.52)
        assertNull(forwardArbitrage(34.0, 100.0, 112.0, 38.08))
        near(impliedForeignRate(1.6, 1.5, 0.05), 0.12)
    }

    @Test fun options() {
        val o = OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
        near(o.price(20.0, 0.0), 150.0 / 11); near(o.price(30.0, 0.0), 225.0 / 11); near(o.price(10.0, 0.0), 75.0 / 11)
        near(OneStepMarket(100.0, 105.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0), 50 - 4000.0 / 105)
        near(OneStepMarket(100.0, 115.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0), 50 - 4000.0 / 115)
        near(o.price(0.0, 20.0), 50.0 / 11)
    }

    @Test fun hedging() {
        val h = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
        near(h.price(60.0, 0.0), 350.0 / 11)
        near(hedgedPurchaseCost(h, 100.0, 1.0, State.Up), 135.0); near(hedgedPurchaseCost(h, 100.0, 1.0, State.Down), 75.0)
        near(hedgedPurchaseCost(h, 100.0, 2.0, State.Up), 110.0); near(hedgedPurchaseCost(h, 100.0, 2.0, State.Down), 110.0)
        for (p in listOf(0.1, 0.25, 0.5, 0.75, 0.9)) {
            val hp = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
            near(hp.stddev(hedgedPurchaseCost(hp, 100.0, 1.0, State.Up), hedgedPurchaseCost(hp, 100.0, 1.0, State.Down)),
                hp.stddev(160.0, 40.0) / 2)
        }
    }

    @Test fun arbitrage() {
        assertNull(m.arbitrage())
        for (S0 in listOf(55.0, 36.0)) {
            val bad = OneStepMarket(100.0, 110.0, S0, 60.0, 40.0, 0.8)
            val P = assertNotNull(bad.arbitrage())
            near(bad.value0(P), 0.0)
            val vu = bad.value1(P, State.Up); val vd = bad.value1(P, State.Down)
            assertTrue(vu >= 0 && vd >= 0 && vu + vd > 0)
        }
    }
}
