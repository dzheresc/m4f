package m4f.ch01

import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class OneStepMarketTest {
    private val market = OneStepMarket(100.0, 110.0, 50.0, 60.0, 40.0, 0.8)

    private fun assertNear(expected: Double, actual: Double) =
        assertTrue(abs(expected - actual) < 1e-9, "expected $expected, got $actual")

    @Test
    fun returns() {
        assertNear(0.1, market.r)
        assertNear(0.2, market.u)
        assertNear(-0.2, market.d)
        assertTrue(market.isArbitrageFree)
    }

    @Test
    fun riskNeutral() {
        assertNear(0.75, market.riskNeutralProb)
    }

    @Test
    fun forward() {
        assertNear(55.0, market.forwardPrice)
    }

    @Test
    fun callReplication() {
        val call = market.replicate(10.0, 0.0)
        assertNear(0.5, call.x)
        assertNear(-2.0 / 11, call.y)
        assertNear(75.0 / 11, market.price(10.0, 0.0))
        assertNear(market.riskNeutralPrice(10.0, 0.0), market.price(10.0, 0.0))
    }

    @Test
    fun putCallParity() {
        assertNear(50 - 50 / 1.1, market.price(10.0, 0.0) - market.price(0.0, 10.0))
    }

    @Test
    fun bookExamplesAndExercises() {
        val ex = OneStepMarket(100.0, 110.0, 50.0, 52.0, 48.0, 0.5)
        assertNear(0.04, ex.u)
        assertNear(-0.04, ex.d)
        assertFalse(ex.isArbitrageFree)
        val portfolio = Portfolio(20.0, 10.0)
        assertNear(2000.0, ex.value0(portfolio))
        assertNear(2140.0, ex.value1(portfolio, State.UP))
        assertNear(2060.0, ex.value1(portfolio, State.DOWN))
        assertNear(0.07, ex.portfolioReturn(portfolio, State.UP))
        assertNear(0.03, ex.portfolioReturn(portfolio, State.DOWN))

        val er = OneStepMarket(90.0, 100.0, 25.0, 30.0, 20.0, 0.5)
        val other = Portfolio(10.0, 15.0)
        assertNear(1600.0, er.value0(other))
        assertNear(1800.0, er.value1(other, State.UP))
        assertNear(1700.0, er.value1(other, State.DOWN))
        val replica = er.replicate(1160.0, 1040.0)
        assertNear(12.0, replica.x)
        assertNear(8.0, replica.y)
        assertNear(1020.0, er.value0(replica))
    }

    @Test
    fun currencyArbitrage() {
        val fx = FxMarket(
            listOf(
                FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
                FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
                FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
                FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
            ),
        )
        val cycle = fx.bestCycle("USD")
        assertEquals(listOf("GBP", "EUR", "USD"), cycle.legs.map { it.to })
        assertNear(1.0202 / (1.5844 * 0.6401), cycle.growth)
    }

    /** Guards the best-leg table that FxMarket builds for speed. */
    @Test
    fun fxLegLookup() {
        val fx = FxMarket(
            listOf(
                FxQuote("A", "EUR", "USD", 1.10, 1.20),
                FxQuote("B", "EUR", "USD", 1.10, 1.15),
                FxQuote("C", "GBP", "USD", 1.30, 1.40),
            ),
        )
        val sellEur = fx.bestLeg("EUR", "USD")!!
        assertEquals("A", sellEur.dealer) // Tie goes to the first dealer.
        assertNear(1.10, sellEur.rate)
        val buyEur = fx.bestLeg("USD", "EUR")!!
        assertEquals("B", buyEur.dealer) // Best rate wins.
        assertNear(1 / 1.15, buyEur.rate)
        assertEquals(null, fx.bestLeg("EUR", "GBP"))
        assertEquals(null, fx.bestLeg("EUR", "JPY"))
        assertEquals(null, fx.bestLeg("JPY", "EUR"))
        assertEquals(FxCycle(emptyList(), 0.0), fx.bestCycle("JPY"))
    }

    @Test
    fun example14() {
        val m4 = OneStepMarket(100.0, 110.0, 100.0, 125.0, 105.0, 0.5)
        assertNear(0.25, m4.u)
        assertNear(0.05, m4.d)
        assertTrue(m4.isArbitrageFree)
        assertNear(0.25, m4.riskNeutralProb)
        assertNear(110.0, m4.forwardPrice)
    }

    @Test
    fun riskAndReturn() {
        val rr = OneStepMarket(100.0, 110.0, 80.0, 100.0, 60.0, 0.8)

        fun assertMoments(portfolio: Portfolio, mean: Double, sd: Double) {
            val kUp = rr.portfolioReturn(portfolio, State.UP)
            val kDown = rr.portfolioReturn(portfolio, State.DOWN)
            assertNear(mean, rr.expect(kUp, kDown))
            assertNear(sd, rr.stddev(kUp, kDown))
        }

        assertMoments(Portfolio(50.0, 60.0), 0.12, 0.08)
        assertMoments(Portfolio(0.0, 100.0), 0.10, 0.0)
        assertMoments(Portfolio(125.0, 0.0), 0.15, 0.20)
        val half = rr.split(10000.0, 0.5)
        assertNear(62.5, half.x)
        assertNear(50.0, half.y)
        assertMoments(half, 0.125, 0.10)
    }

    @Test
    fun forwards() {
        val arb = assertNotNull(forwardArbitrage(34.0, 100.0, 112.0, 38.60))
        assertNear(0.0, arb.value0(34.0, 100.0))
        assertNear(0.52, arb.value1(30.0, 112.0, 38.60))
        assertNear(0.52, arb.value1(50.0, 112.0, 38.60))
        assertNull(forwardArbitrage(34.0, 100.0, 112.0, 38.08))
        assertNear(0.12, impliedForeignRate(1.6, 1.5, 0.05))
    }

    @Test
    fun options() {
        val o = OneStepMarket(100.0, 110.0, 100.0, 120.0, 80.0, 0.5)
        assertNear(150.0 / 11, o.price(20.0, 0.0))
        assertNear(225.0 / 11, o.price(30.0, 0.0))
        assertNear(75.0 / 11, o.price(10.0, 0.0))
        assertNear(
            50 - 4000.0 / 105,
            OneStepMarket(100.0, 105.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0),
        )
        assertNear(
            50 - 4000.0 / 115,
            OneStepMarket(100.0, 115.0, 100.0, 120.0, 80.0, 0.5).price(20.0, 0.0),
        )
        assertNear(50.0 / 11, o.price(0.0, 20.0))
    }

    @Test
    fun hedging() {
        val h = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, 0.5)
        assertNear(350.0 / 11, h.price(60.0, 0.0))
        assertNear(135.0, hedgedPurchaseCost(h, 100.0, 1.0, State.UP))
        assertNear(75.0, hedgedPurchaseCost(h, 100.0, 1.0, State.DOWN))
        assertNear(110.0, hedgedPurchaseCost(h, 100.0, 2.0, State.UP))
        assertNear(110.0, hedgedPurchaseCost(h, 100.0, 2.0, State.DOWN))
        for (p in listOf(0.1, 0.25, 0.5, 0.75, 0.9)) {
            val hp = OneStepMarket(100.0, 110.0, 100.0, 160.0, 40.0, p)
            val sigmaWith = hp.stddev(
                hedgedPurchaseCost(hp, 100.0, 1.0, State.UP),
                hedgedPurchaseCost(hp, 100.0, 1.0, State.DOWN),
            )
            assertNear(hp.stddev(160.0, 40.0) / 2, sigmaWith)
        }
    }

    @Test
    fun arbitrage() {
        assertNull(market.arbitrage())
        for (s0 in listOf(55.0, 36.0)) {
            val bad = OneStepMarket(100.0, 110.0, s0, 60.0, 40.0, 0.8)
            val arb = assertNotNull(bad.arbitrage())
            assertNear(0.0, bad.value0(arb))
            val vUp = bad.value1(arb, State.UP)
            val vDown = bad.value1(arb, State.DOWN)
            assertTrue(vUp >= 0 && vDown >= 0 && vUp + vDown > 0)
        }
    }
}
