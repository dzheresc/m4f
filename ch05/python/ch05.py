#!/usr/bin/env python3
"""Chapter 5 worked examples. Output is byte-identical to the C++ and Kotlin versions.

The numbers are illustrative (not the book's own); the formulas follow the book.
"""
from portfolio import (beta, capital_market_line, covariance_matrix, market_portfolio,
                       min_variance_line, min_variance_portfolio, portfolio_mean, portfolio_std,
                       security_market_line)
from risk import (correlation, covariance, mean, min_variance_weight, std_dev, two_asset_mean,
                  two_asset_std)


def f(v: float) -> str:
    return f"{v + 0.0:.4f}"


def fl(vs: list[float]) -> str:
    return "[" + ", ".join(f(v) for v in vs) + "]"


def report() -> str:
    out: list[str] = []
    line = out.append
    line("Chapter 5: Portfolio Management (illustrative numbers, book formulas)")

    line("\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==")
    p = [0.2, 0.5, 0.3]
    K1, K2 = [-0.10, 0.10, 0.20], [0.20, 0.05, -0.05]
    line(f"K1={fl(K1)}: mu={f(mean(K1, p))} sigma={f(std_dev(K1, p))}")
    line(f"K2={fl(K2)}: mu={f(mean(K2, p))} sigma={f(std_dev(K2, p))}")
    line(f"Cov(K1,K2)={f(covariance(K1, K2, p))} rho={f(correlation(K1, K2, p))}")
    KV = [0.5 * a + 0.5 * b for a, b in zip(K1, K2)]
    rho = correlation(K1, K2, p)
    line(f"Half and half: sigma from scenarios={f(std_dev(KV, p))}, "
         f"from formula={f(two_asset_std(0.5, std_dev(K1, p), std_dev(K2, p), rho))}")

    line("\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 sigma2=0.20 ==")
    for r in (-1.0, -0.5, 0.0, 0.5, 1.0):
        w = min_variance_weight(0.10, 0.20, r)
        line(f"rho={f(r)}: min variance w1={f(w)} mu={f(two_asset_mean(w, 0.08, 0.12))} "
             f"sigma={f(two_asset_std(w, 0.10, 0.20, r))}; w1=0.4: sigma={f(two_asset_std(0.4, 0.10, 0.20, r))}")

    line("\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) sigma=(0.15, 0.25, 0.30) ==")
    m = [0.10, 0.15, 0.20]
    C = covariance_matrix([0.15, 0.25, 0.30], [[1.0, 0.3, 0.1], [0.3, 1.0, 0.4], [0.1, 0.4, 1.0]])
    line("rho12=0.3 rho13=0.1 rho23=0.4")
    w = [0.4, 0.3, 0.3]
    line(f"w={fl(w)}: mu={f(portfolio_mean(w, m))} sigma={f(portfolio_std(w, C))}")

    line("\n== 5.3.2 Efficient frontier ==")
    wmv = min_variance_portfolio(C)
    line(f"Minimum variance portfolio w={fl(wmv)} mu={f(portfolio_mean(wmv, m))} sigma={f(portfolio_std(wmv, C))}")
    for mu in (0.15, 0.18, 0.21):
        wl = min_variance_line(m, C, mu)
        line(f"mu_V={f(mu)}: w={fl(wl)} sigma={f(portfolio_std(wl, C))}")

    line("\n== 5.4.1 Capital market line: R=0.05 ==")
    wM = market_portfolio(m, C, 0.05)
    muM, sM = portfolio_mean(wM, m), portfolio_std(wM, C)
    line(f"Market portfolio w_M={fl(wM)} mu_M={f(muM)} sigma_M={f(sM)} slope={f((muM - 0.05) / sM)}")
    line(f"On the CML at sigma=0.15: mu={f(capital_market_line(0.15, 0.05, muM, sM))}")

    line("\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==")
    betas = [beta([1.0 if j == i else 0.0 for j in range(3)], wM, C) for i in range(3)]
    line(f"beta={fl(betas)}")
    line(f"w={fl(w)}: beta_V={f(beta(w, wM, C))} sum w_i beta_i={f(sum(a * b for a, b in zip(w, betas)))}")

    line("\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V ==")
    for i in range(3):
        line(f"security {i + 1}: beta={f(betas[i])} SML mu={f(security_market_line(betas[i], 0.05, muM))} actual mu={f(m[i])}")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    print(report(), end="")
