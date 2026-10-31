plugins {
    kotlin("jvm") version "2.1.21"
    application
    id("me.champeau.jmh") version "0.7.2"
}

repositories { mavenCentral() }

dependencies { testImplementation(kotlin("test")) }

kotlin { jvmToolchain(21) }

application { mainClass.set("m4f.ch05.MainKt") }

tasks.test { useJUnitPlatform() }

// Microbenchmarks in src/jmh: ../bench.sh --kotlin [regex]. Average time per call, plus bytes
// allocated per call from the GC profiler.
jmh {
    warmupIterations.set(3)
    iterations.set(5)
    warmup.set("1s")
    timeOnIteration.set("1s")
    fork.set(1)
    benchmarkMode.set(listOf("avgt"))
    timeUnit.set("us")
    profilers.set(listOf("gc"))
    resultFormat.set("JSON")
    includes.set(listOf(project.findProperty("bench")?.toString() ?: ".*"))
}
