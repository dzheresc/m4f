#include "m4f/derivatives.hpp"
#include "m4f/rate_tree.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch11;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

int main() {
    const int N = 4;
    const double tau = 1.0;
    const Lattice rates = rateLattice(0.05, 0.01, -0.01, N);
    const Lattice probs = constantProbs(0.5, N);

    check(rates.size() == 5 && near(rates[2][2], 0.07) && near(rates[2][0], 0.03), "rate lattice");
    const auto A = moneyMarket(rates, tau, "ud");
    check(near(A[1], std::exp(0.05)) && near(A[2], std::exp(0.05 + 0.06)), "money market");
    check(near(oneStepPrice(0.05, 0.5), std::exp(-0.025)), "one-step price");

    const Lattice B1 = bondLattice(rates, probs, tau, 1);
    check(near(B1[0][0], std::exp(-0.05)), "B(0,1)");
    const Lattice B2 = bondLattice(rates, probs, tau, 2);
    check(near(B2[0][0], std::exp(-0.05) * (0.5 * std::exp(-0.06) + 0.5 * std::exp(-0.04))), "B(0,2)");
    const auto B0 = bondPrices(rates, probs, tau, N);
    check(B0.size() == 4, "4 prices");
    for (int m = 1; m <= N; ++m) check(near(B0[m - 1], bondLattice(rates, probs, tau, m)[0][0]), "bond prices");
    for (std::size_t i = 1; i < B0.size(); ++i) check(B0[i - 1] > B0[i], "decreasing in maturity");
    check(near(yieldRate(B1[0][0], 0, 1, tau), 0.05), "yield");
    const Lattice B4 = bondLattice(rates, probs, tau, N);
    for (int n = 0; n < N; ++n)
        for (int k = 0; k <= n; ++k)
            check(near(B4[n][k] * std::exp(rates[n][k] * tau), 0.5 * B4[n + 1][k + 1] + 0.5 * B4[n + 1][k]), "martingale");

    for (double p : {0.5, 0.6, 0.35}) {
        const Lattice B = bondLattice(rates, constantProbs(p, N), tau, N);
        for (const auto& row : impliedProbs(B, rates, tau))
            for (double q : row) check(near(q, p), "implied p*");
    }
    check(near(impliedProb(B2, rates, tau, 0, 0), 0.5), "implied p* from B2");

    const Lattice C = bondOption(rates, probs, tau, N, 2, 0.9, true);
    const Lattice P = bondOption(rates, probs, tau, N, 2, 0.9, false);
    check(near(C[0][0] - P[0][0], B4[0][0] - 0.9 * B2[0][0]), "put-call parity");
    const Lattice PA = bondOption(rates, probs, tau, N, 2, 0.9, false, true);
    check(PA[0][0] >= P[0][0] && near(PA[0][0], 0.9 - B4[0][0]), "American put");
    check(near(bondOption(rates, probs, tau, N, 2, 0.0, true)[0][0], B4[0][0]), "zero-strike call = bond");

    const double K = swapRate(B0);
    check(near(payerSwap(rates, probs, tau, N, K)[0][0], 0.0), "swap worthless at swap rate");
    check(near(payerSwap(rates, probs, tau, N, 0.0)[0][0], 1 - B0.back()), "floating leg = 1 - B(0,N)");
    for (double Kx : {0.03, K, 0.07}) {
        const double c = cap(rates, probs, tau, N, Kx)[0][0], fo = floor(rates, probs, tau, N, Kx)[0][0];
        check(near(c - fo, payerSwap(rates, probs, tau, N, Kx)[0][0]), "cap - floor = swap");
        check(c >= 0.0 && fo >= 0.0, "nonnegative");
    }
    double sum = 0.0;
    for (int n = 0; n < N; ++n) sum += caplet(rates, probs, tau, n, 0.05);
    check(near(sum, cap(rates, probs, tau, N, 0.05)[0][0]), "caplets sum to cap");
    check(near(caplet(rates, probs, tau, 0, 0.05), std::exp(-0.05) * std::max(floatingRate(rates, tau, 0, 0) - 0.05, 0.0)), "first caplet");

    const Lattice V = discountBack(rates, probs, tau, std::vector<double>(N + 1, 0.0), [](int, int) { return 1.0; });
    double sb = 0.0;
    for (double b : B0) sb += b;
    check(near(V[0][0], sb), "annuity = sum of bonds");

    if (failures == 0) std::printf("All C++ tests passed\n");
    return failures == 0 ? 0 : 1;
}
