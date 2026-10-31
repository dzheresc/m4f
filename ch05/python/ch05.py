#!/usr/bin/env python3
"""Chapter 5 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import portfolio
import risk

# The three-security market shared by sections 5.3 and 5.4.
MEANS = [0.10, 0.15, 0.20]
WEIGHTS = [0.4, 0.3, 0.3]
RISK_FREE = 0.05


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def unit(i: int) -> list[float]:
    """The i-th unit vector of length 3."""
    return [1.0 if j == i else 0.0 for j in range(3)]


def market_covariance() -> portfolio.Mat:
    """Covariance matrix of the three securities."""
    return portfolio.covariance_matrix(
        [0.15, 0.25, 0.30],
        [[1.0, 0.3, 0.1], [0.3, 1.0, 0.4], [0.1, 0.4, 1.0]],
    )


def risk_section(out: list[str]) -> None:
    """Section 5.1."""
    out.append("\n== 5.1 Risk: scenarios with p = {0.2, 0.5, 0.3} ==")
    p = [0.2, 0.5, 0.3]
    k1, k2 = [-0.10, 0.10, 0.20], [0.20, 0.05, -0.05]
    out.append(
        f"K1={fmt_list(k1)}: mu={fmt(risk.mean(k1, p))} "
        f"sigma={fmt(risk.std_dev(k1, p))}"
    )
    out.append(
        f"K2={fmt_list(k2)}: mu={fmt(risk.mean(k2, p))} "
        f"sigma={fmt(risk.std_dev(k2, p))}"
    )
    out.append(
        f"Cov(K1,K2)={fmt(risk.covariance(k1, k2, p))} "
        f"rho={fmt(risk.correlation(k1, k2, p))}"
    )
    k_v = [0.5 * a + 0.5 * b for a, b in zip(k1, k2)]
    rho = risk.correlation(k1, k2, p)
    formula = risk.two_asset_std(
        0.5, risk.std_dev(k1, p), risk.std_dev(k2, p), rho
    )
    out.append(
        f"Half and half: sigma from scenarios={fmt(risk.std_dev(k_v, p))}, "
        f"from formula={fmt(formula)}"
    )


def two_securities_section(out: list[str]) -> None:
    """Section 5.2.1."""
    out.append(
        "\n== 5.2.1 Two securities: mu1=0.08 sigma1=0.10, mu2=0.12 "
        "sigma2=0.20 =="
    )
    for rho in (-1.0, -0.5, 0.0, 0.5, 1.0):
        w = risk.min_variance_weight(0.10, 0.20, rho)
        out.append(
            f"rho={fmt(rho)}: min variance w1={fmt(w)} "
            f"mu={fmt(risk.two_asset_mean(w, 0.08, 0.12))} "
            f"sigma={fmt(risk.two_asset_std(w, 0.10, 0.20, rho))}; "
            f"w1=0.4: sigma={fmt(risk.two_asset_std(0.4, 0.10, 0.20, rho))}"
        )


def several_securities_section(out: list[str], cov: portfolio.Mat) -> None:
    """Section 5.3.1."""
    out.append(
        "\n== 5.3.1 Several securities: m=(0.10, 0.15, 0.20) "
        "sigma=(0.15, 0.25, 0.30) =="
    )
    out.append("rho12=0.3 rho13=0.1 rho23=0.4")
    out.append(
        f"w={fmt_list(WEIGHTS)}: "
        f"mu={fmt(portfolio.portfolio_mean(WEIGHTS, MEANS))} "
        f"sigma={fmt(portfolio.portfolio_std(WEIGHTS, cov))}"
    )


def efficient_frontier_section(out: list[str], cov: portfolio.Mat) -> None:
    """Section 5.3.2."""
    out.append("\n== 5.3.2 Efficient frontier ==")
    w_min = portfolio.min_variance_portfolio(cov)
    out.append(
        f"Minimum variance portfolio w={fmt_list(w_min)} "
        f"mu={fmt(portfolio.portfolio_mean(w_min, MEANS))} "
        f"sigma={fmt(portfolio.portfolio_std(w_min, cov))}"
    )
    for mu in (0.15, 0.18, 0.21):
        w = portfolio.min_variance_line(MEANS, cov, mu)
        out.append(
            f"mu_V={fmt(mu)}: w={fmt_list(w)} "
            f"sigma={fmt(portfolio.portfolio_std(w, cov))}"
        )


def capm_section(out: list[str], cov: portfolio.Mat) -> None:
    """Sections 5.4.1-5.4.3."""
    out.append("\n== 5.4.1 Capital market line: R=0.05 ==")
    w_market = portfolio.market_portfolio(MEANS, cov, RISK_FREE)
    mu_m = portfolio.portfolio_mean(w_market, MEANS)
    sigma_m = portfolio.portfolio_std(w_market, cov)
    out.append(
        f"Market portfolio w_M={fmt_list(w_market)} mu_M={fmt(mu_m)} "
        f"sigma_M={fmt(sigma_m)} slope={fmt((mu_m - RISK_FREE) / sigma_m)}"
    )
    cml = portfolio.capital_market_line(0.15, RISK_FREE, mu_m, sigma_m)
    out.append(f"On the CML at sigma=0.15: mu={fmt(cml)}")

    out.append("\n== 5.4.2 Beta factor: beta_V = Cov(K_V, K_M) / sigma_M^2 ==")
    betas = [portfolio.beta(unit(i), w_market, cov) for i in range(3)]
    out.append(f"beta={fmt_list(betas)}")
    weighted_beta = sum(a * b for a, b in zip(WEIGHTS, betas))
    out.append(
        f"w={fmt_list(WEIGHTS)}: "
        f"beta_V={fmt(portfolio.beta(WEIGHTS, w_market, cov))} "
        f"sum w_i beta_i={fmt(weighted_beta)}"
    )

    out.append(
        "\n== 5.4.3 Security market line: mu_V = R + (mu_M - R) beta_V =="
    )
    for i in range(3):
        sml = portfolio.security_market_line(betas[i], RISK_FREE, mu_m)
        out.append(
            f"security {i + 1}: beta={fmt(betas[i])} SML mu={fmt(sml)} "
            f"actual mu={fmt(MEANS[i])}"
        )


def report() -> str:
    """The full chapter 5 report."""
    out = [
        "Chapter 5: Portfolio Management (illustrative numbers, book formulas)"
    ]
    cov = market_covariance()
    risk_section(out)
    two_securities_section(out)
    several_securities_section(out, cov)
    efficient_frontier_section(out, cov)
    capm_section(out, cov)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
