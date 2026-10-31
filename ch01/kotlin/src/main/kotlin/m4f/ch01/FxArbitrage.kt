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
    private val index = currencies.withIndex().associate { (i, c) -> c to i }

    // n x n table, row = from, column = to: index into legs of the best leg, or -1. Built once
    // so that the search does no string comparisons. Legs are scanned in order and only a
    // strictly better rate replaces the current best, so the first dealer wins ties.
    private val bestLegIndex = IntArray(currencies.size * currencies.size) { -1 }.also { table ->
        for ((k, leg) in legs.withIndex()) {
            val cell = index.getValue(leg.from) * currencies.size + index.getValue(leg.to)
            if (table[cell] < 0 || leg.rate > legs[table[cell]].rate) table[cell] = k
        }
    }

    /** Best direct conversion [from] -> [to] (first dealer wins ties). */
    fun bestLeg(from: String, to: String): FxLeg? {
        val i = index[from] ?: return null
        val j = index[to] ?: return null
        val k = bestLegIndex[i * currencies.size + j]
        return if (k < 0) null else legs[k]
    }

    /**
     * Most profitable round trip [start] -> ... -> [start] through distinct currencies
     * (exhaustive search over currency and leg indices; fine for a handful of currencies).
     */
    fun bestCycle(start: String): FxCycle {
        val s = index[start] ?: return FxCycle(emptyList(), 0.0)
        val n = currencies.size
        val visited = BooleanArray(n)
        val path = IntArray(n)
        var depth = 0
        var bestPath = IntArray(0)
        var bestGrowth = 0.0

        fun search(at: Int, growth: Double) {
            for (next in 0 until n) {
                val k = bestLegIndex[at * n + next]
                if (k < 0) continue
                val closes = next == s
                if (!closes && visited[next]) continue
                if (closes && depth == 0) continue
                path[depth++] = k
                val newGrowth = growth * legs[k].rate
                if (closes) {
                    if (newGrowth > bestGrowth) {
                        bestGrowth = newGrowth
                        bestPath = path.copyOf(depth)
                    }
                } else {
                    visited[next] = true
                    search(next, newGrowth)
                    visited[next] = false
                }
                depth--
            }
        }
        visited[s] = true
        search(s, 1.0)
        return FxCycle(bestPath.map { legs[it] }, bestGrowth)
    }
}
