// Microbenchmarks for chapter 9 (JMH). Run from the chapter directory as ./bench.sh --kotlin.
package m4f.ch09

import org.openjdk.jmh.annotations.Benchmark
import org.openjdk.jmh.annotations.Scope
import org.openjdk.jmh.annotations.State

@State(Scope.Benchmark)
open class Ch09Benchmark {
    private val model = Lognormal(100.0, 0.12, 0.25, 0.5)
    private val sample = mix(
        "mix",
        allInStock(10000.0, 100.0),
        0.5,
        bullSpread("bull", 10000.0, 100.0, 100.0, 120.0, 0.05, 0.25, 0.5),
        0.5,
    )

    // A field, so the JIT cannot fold the call into a constant.
    private var p = 0.05

    @Benchmark
    fun normInv() = normInv(p)

    @Benchmark
    fun bsGreeksCall() = bsGreeks(100.0, 100.0, 0.05, 0.2, 0.5, true)

    @Benchmark
    fun bsGreeksPut() = bsGreeks(100.0, 100.0, 0.05, 0.2, 0.5, false)

    @Benchmark
    fun valueAtRisk() = valueAtRisk(sample, model, 0.05, 0.25, 0.95)

    @Benchmark
    fun report() = m4f.ch09.report()
}
