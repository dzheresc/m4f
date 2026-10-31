// Microbenchmarks for chapter 1 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch01

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Param
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.Setup
import org.openjdk.jmh.annotations.State
import kotlin.random.Random

/** Exercise 1.3: dealers A (New York) and B (London). */
private fun bookQuotes() = listOf(
    FxQuote("A", "EUR", "USD", 1.0202, 1.0284),
    FxQuote("A", "GBP", "USD", 1.5718, 1.5844),
    FxQuote("B", "EUR", "GBP", 0.6324, 0.6401),
    FxQuote("B", "USD", "GBP", 0.6299, 0.6375),
)

/** One dealer quoting every pair of [n] currencies, 0.5% spread plus noise. */
private fun syntheticQuotes(n: Int): List<FxQuote> {
    val rng = Random(n)
    val mid = List(n) { rng.nextDouble(0.5, 2.0) }
    val quotes = ArrayList<FxQuote>()
    for (i in 0 until n) {
        for (j in i + 1 until n) {
            val rate = mid[i] / mid[j] * (1 + rng.nextDouble(-0.004, 0.004))
            quotes.add(FxQuote("D", "C$i", "C$j", rate * 0.9975, rate * 1.0025))
        }
    }
    return quotes
}

@State(Scope.Benchmark)
open class Ch01Benchmark {
    private val quotes = bookQuotes()
    private val book = FxMarket(quotes)

    @Benchmark
    fun fxMarketConstruct() = FxMarket(quotes)

    @Benchmark
    fun bestLeg() = book.bestLeg("USD", "GBP")

    @Benchmark
    fun bestCycleBook() = book.bestCycle("USD")

    @Benchmark
    fun report() = m4f.ch01.report()
}

@State(Scope.Benchmark)
open class BestCycleScaling {
    @Param("3", "6", "8")
    var n = 0
    private lateinit var market: FxMarket

    @Setup
    fun setup() {
        market = FxMarket(syntheticQuotes(n))
    }

    @Benchmark
    fun bestCycle() = market.bestCycle("C0")
}
