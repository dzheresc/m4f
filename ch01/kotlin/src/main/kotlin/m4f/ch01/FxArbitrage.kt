// Exercise 1.3: spotting a risk-free profit among currency dealers' quotes.
package m4f.ch01

/**
 * A quote for [base] in [quote] currency: the dealer buys 1 base for [buy] units of quote and
 * sells 1 base for [sell] units of quote (buy < sell).
 */
data class FxQuote(
    val dealer: String,
    val base: String,
    val quote: String,
    val buy: Double,
    val sell: Double,
)

/** Converting 1 unit of [from] into [rate] units of [to] with [dealer]. */
data class FxLeg(val from: String, val to: String, val dealer: String, val rate: Double)

/** A round trip; [growth] is units of start currency received per unit invested. */
data class FxCycle(val legs: List<FxLeg>, val growth: Double)

class FxMarket(quotes: List<FxQuote>) {
    // Sell base to the dealer, or buy base from the dealer.
    private val legs = quotes.flatMap {
        listOf(
            FxLeg(it.base, it.quote, it.dealer, it.buy),
            FxLeg(it.quote, it.base, it.dealer, 1.0 / it.sell),
        )
    }
    private val currencies = quotes.flatMap { listOf(it.base, it.quote) }.distinct().sorted()

    /** Best direct conversion [from] -> [to] (first dealer wins ties). */
    fun bestLeg(from: String, to: String): FxLeg? {
        var best: FxLeg? = null
        for (leg in legs) {
            if (leg.from == from && leg.to == to && (best == null || leg.rate > best.rate)) {
                best = leg
            }
        }
        return best
    }

    /**
     * Most profitable round trip [start] -> ... -> [start] through distinct currencies
     * (exhaustive search; fine for a handful of currencies).
     */
    fun bestCycle(start: String): FxCycle {
        var best = FxCycle(emptyList(), 0.0)
        val path = ArrayList<FxLeg>()
        val visited = mutableListOf(start)

        fun search(at: String, growth: Double) {
            for (next in currencies) {
                val leg = bestLeg(at, next) ?: continue
                val closes = next == start
                if (!closes && next in visited) continue
                if (closes && path.isEmpty()) continue
                path.add(leg)
                val newGrowth = growth * leg.rate
                if (closes) {
                    if (newGrowth > best.growth) best = FxCycle(path.toList(), newGrowth)
                } else {
                    visited.add(next)
                    search(next, newGrowth)
                    visited.removeAt(visited.lastIndex)
                }
                path.removeAt(path.lastIndex)
            }
        }
        search(start, 1.0)
        return best
    }
}
