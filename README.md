# Mathematics for Finance: code companion

Code for Capinski & Zastawniak, *Mathematics for Finance*. Each chapter is implemented in C++20 (CMake), Kotlin (Gradle) and Python 3 (standard library), and all three produce the same report.

| Chapter | Topic |
|---|---|
| [ch01](ch01/) | Introduction: one-step model, forwards, options |
| [ch02](ch02/) | Risk-free assets: interest, bonds, money market |
| [ch03](ch03/) | Risky assets: returns, binomial and trinomial trees, continuous-time limit |
| [ch04](ch04/) | Discrete time market models: strategies, no arbitrage, FTAP |
| [ch05](ch05/) | Portfolio management: risk, efficient frontier, CAPM |
| [ch06](ch06/) | Forward and futures contracts: pricing, value, margin, hedging |
| [ch07](ch07/) | Options: payoffs, put-call parity, bounds, time value |

Run `./run_all.sh` inside a chapter to build, test and diff all three languages.
