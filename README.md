# Mathematics for Finance in C++, Kotlin, Python and Rust

Working code for every chapter of Capiński & Zastawniak, *Mathematics for Finance: An Introduction to Financial Engineering*. Each chapter is implemented in C++20, Kotlin, Python 3 and Rust, and all of them print the same report. (Rust is being added chapter by chapter; a chapter with a `rust/` directory has it.)

Builds: CMake for C++, Gradle for Kotlin, Cargo for Rust; the Python code uses only the standard library.

| Chapter | Topic |
|---|---|
| [ch01](ch01/) | Introduction: one-step model, forwards, options |
| [ch02](ch02/) | Risk-free assets: interest, bonds, money market |
| [ch03](ch03/) | Risky assets: returns, binomial and trinomial trees, continuous-time limit |
| [ch04](ch04/) | Discrete time market models: strategies, no arbitrage, FTAP |
| [ch05](ch05/) | Portfolio management: risk, efficient frontier, CAPM |
| [ch06](ch06/) | Forward and futures contracts: pricing, value, margin, hedging |
| [ch07](ch07/) | Options: payoffs, put-call parity, bounds, time value |
| [ch08](ch08/) | Option pricing: binomial trees, CRR formula, American options, Black-Scholes |
| [ch09](ch09/) | Financial engineering: delta/gamma/vega hedging, Greeks, Value at Risk, speculation |
| [ch10](ch10/) | Variable interest rates: duration, immunisation, dynamic hedging, forward rates, money market |
| [ch11](ch11/) | Stochastic interest rates: short-rate tree, bond pricing, risk-neutral probabilities, bond options, swaps, caps and floors |

Run `./run_all.sh` inside a chapter to build, test and diff every language.

Benchmarks and before/after results: [C++](PERFORMANCE.md), [Kotlin](PERFORMANCE_KOTLIN.md), [Python](PERFORMANCE_PYTHON.md); Rust vs C++: [Rust](PERFORMANCE_RUST.md). Each chapter runs them with `./bench.sh`, `./bench.sh --kotlin`, `./bench.sh --python` or `./bench.sh --rust`; `./bench.sh --compare` prints a Rust vs C++ table.

Code style: Google style guides (Android style for Kotlin; rustfmt and clippy for Rust, which has no Google guide), see [STYLE.md](STYLE.md). Chapters are converted one at a time; a converted chapter has a `check_style.sh`.
