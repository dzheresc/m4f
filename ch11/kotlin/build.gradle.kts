plugins {
    kotlin("jvm") version "2.1.21"
    application
}

repositories { mavenCentral() }

dependencies { testImplementation(kotlin("test")) }

kotlin { jvmToolchain(21) }

application { mainClass.set("m4f.ch11.MainKt") }

tasks.test { useJUnitPlatform() }
