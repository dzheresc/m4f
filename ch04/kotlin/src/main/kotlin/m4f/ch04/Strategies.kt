package m4f.ch04

// Section 4.1, stock and money market models: investment strategies.
//
// Prices are given along one path: S[n] (or S[j][n] for several stocks) and A[n]
// for n = 0..N. A strategy holds x(n) shares and y(n) units of the money market
// over the step from n-1 to n, chosen at time n-1; lists are indexed from 0, so
// x[0] is x(1).

/** y[n-1] = y(n), V[n] = V(n) */
data class StrategyPath(val y: List<Double>, val V: List<Double>)

/** Several stocks S[j][n], optional dividends div[j][n] paid at n to the holders
 *  over (n-1, n]. The money market position y(n+1) is set so that rebalancing at
 *  n costs exactly V(n):
 *    sum_j x_j(n+1) S_j(n) + y(n+1) A(n) = V(n),
 *    V(n) = sum_j x_j(n) (S_j(n) + div_j(n)) + y(n) A(n). */
fun selfFinancingMulti(
    V0: Double, S: List<List<Double>>, A: List<Double>, x: List<List<Double>>,
    div: List<List<Double>>? = null,
): StrategyPath {
    val N = A.size - 1
    val V = mutableListOf(V0)
    val y = mutableListOf<Double>()
    for (n in 1..N) {
        var stock = 0.0
        for (j in S.indices) stock += x[j][n - 1] * S[j][n - 1]
        y.add((V[n - 1] - stock) / A[n - 1])
        var value = y[n - 1] * A[n]
        for (j in S.indices) {
            val d = div?.get(j)?.get(n) ?: 0.0
            value += x[j][n - 1] * (S[j][n] + d)
        }
        V.add(value)
    }
    return StrategyPath(y, V)
}

/** One stock: y(n) and V(n) of the self-financing strategy with initial value V0
 *  and stock positions x. */
fun selfFinancing(V0: Double, S: List<Double>, A: List<Double>, x: List<Double>): StrategyPath =
    selfFinancingMulti(V0, listOf(S), A, listOf(x))

/** V(N) - V(0) = sum_n x(n) (S(n) - S(n-1)) + y(n) (A(n) - A(n-1)) */
fun gains(S: List<Double>, A: List<Double>, x: List<Double>, y: List<Double>): Double {
    var g = 0.0
    for (n in 1 until A.size) g += x[n - 1] * (S[n] - S[n - 1]) + y[n - 1] * (A[n] - A[n - 1])
    return g
}

/** V~(N) - V~(0) = sum_n x(n) (S~(n) - S~(n-1)), with S~(n) = S(n) / A(n) */
fun discountedGains(S: List<Double>, A: List<Double>, x: List<Double>): Double {
    var g = 0.0
    for (n in 1 until A.size) g += x[n - 1] * (S[n] / A[n] - S[n - 1] / A[n - 1])
    return g
}
