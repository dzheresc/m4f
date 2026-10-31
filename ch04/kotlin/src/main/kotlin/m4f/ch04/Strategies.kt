// Section 4.1, stock and money market models: investment strategies.
//
// Prices are given along one path: s[n] (or s[j][n] for several stocks) and a[n] for n = 0..N,
// standing for the book's S(n) and A(n). A strategy holds x(n) shares and y(n) units of the money
// market over the step from n-1 to n, chosen at time n-1; lists are indexed from 0, so x[0] is
// x(1).
package m4f.ch04

/** [y] with y[n-1] = y(n) and [values] with values[n] = V(n). */
data class StrategyPath(val y: List<Double>, val values: List<Double>)

/**
 * Several stocks s[j][n], optional [dividends] paid at n to the holders over (n-1, n]. The money
 * market position y(n+1) is set so that rebalancing at n costs exactly V(n):
 * sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n), with
 * V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n).
 */
fun selfFinancingMulti(
    v0: Double,
    s: List<List<Double>>,
    a: List<Double>,
    x: List<List<Double>>,
    dividends: List<List<Double>>? = null,
): StrategyPath {
    val numSteps = a.size - 1
    val values = mutableListOf(v0)
    val y = mutableListOf<Double>()
    for (n in 1..numSteps) {
        var stock = 0.0
        for (j in s.indices) {
            stock += x[j][n - 1] * s[j][n - 1]
        }
        y.add((values[n - 1] - stock) / a[n - 1])
        var value = y[n - 1] * a[n]
        for (j in s.indices) {
            val dividend = dividends?.get(j)?.get(n) ?: 0.0
            value += x[j][n - 1] * (s[j][n] + dividend)
        }
        values.add(value)
    }
    return StrategyPath(y, values)
}

/**
 * One stock: y(n) and V(n) of the self-financing strategy with initial value [v0]. Same
 * arithmetic as selfFinancingMulti with one stock and no dividends, without the inner loops.
 */
fun selfFinancing(v0: Double, s: List<Double>, a: List<Double>, x: List<Double>): StrategyPath {
    val numSteps = a.size - 1
    val values = DoubleArray(numSteps + 1)
    val y = DoubleArray(numSteps)
    values[0] = v0
    for (n in 1..numSteps) {
        y[n - 1] = (values[n - 1] - x[n - 1] * s[n - 1]) / a[n - 1]
        values[n] = y[n - 1] * a[n] + x[n - 1] * s[n]
    }
    return StrategyPath(y.asList(), values.asList())
}

/** V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1)). */
fun gains(s: List<Double>, a: List<Double>, x: List<Double>, y: List<Double>): Double {
    var total = 0.0
    for (n in 1 until a.size) {
        total += x[n - 1] * (s[n] - s[n - 1]) + y[n - 1] * (a[n] - a[n - 1])
    }
    return total
}

/** V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n). */
fun discountedGains(s: List<Double>, a: List<Double>, x: List<Double>): Double {
    var total = 0.0
    for (n in 1 until a.size) {
        total += x[n - 1] * (s[n] / a[n] - s[n - 1] / a[n - 1])
    }
    return total
}
