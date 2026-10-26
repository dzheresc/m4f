#include "m4f/binomial_pricing.hpp"
#include "m4f/black_scholes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace m4f::ch08;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }
static Payoff call(double X) { return [X](double s) { return std::max(s - X, 0.0); }; }
static Payoff put(double X) { return [X](double s) { return std::max(X - s, 0.0); }; }

int main() {
    // 8.1.1
    const Replication rep = oneStepReplication(100, 0.2, -0.1, 0.1, 20, 0);
    check(near(rep.x * 120 + rep.y * 1.1, 20) && near(rep.x * 90 + rep.y * 1.1, 0), "one-step replication");
    check(near(rep.price, 40 / 3.3), "one-step price");

    // 8.1.2 the replicating position reproduces both successor values
    const Tree V = optionTree(100, 0.2, -0.1, 0.1, 3, call(100));
    for (int n = 0; n < 3; ++n)
        for (int k = 0; k <= n; ++k) {
            const auto [x, y] = replicatingPosition(100, 0.2, -0.1, 0.1, V, n, k);
            for (int kk : {k, k + 1})
                check(near(x * stockPrice(100, 0.2, -0.1, n + 1, kk) + y * std::pow(1.1, n + 1), V[n + 1][kk]),
                      "replication");
        }

    // 8.1.3-8.1.4
    for (int N : {1, 2, 5, 8})
        for (double X : {70.0, 100.0, 130.0}) {
            const double tree = optionTree(100, 0.2, -0.1, 0.1, N, call(X))[0][0];
            check(near(europeanPriceSum(100, 0.2, -0.1, 0.1, N, call(X)), tree), "sum formula");
            check(near(crrCall(100, X, 0.2, -0.1, 0.1, N).price, tree), "CRR formula");
            const double P = optionTree(100, 0.2, -0.1, 0.1, N, put(X))[0][0];
            check(near(tree - P, 100 - X * std::pow(1.1, -N)), "put-call parity");
        }
    check(near(binomialSum(0.3, 6, 0), 1), "binomial sum");

    // 8.2
    const Tree VA = optionTree(100, 0.1, -0.1, 0.02, 4, put(100), true);
    const Tree VE = optionTree(100, 0.1, -0.1, 0.02, 4, put(100));
    check(VA[0][0] > VE[0][0], "P_A > P_E");
    for (int n = 0; n < 5; ++n)
        for (int k = 0; k <= n; ++k)
            check(VA[n][k] >= std::max(100 - stockPrice(100, 0.1, -0.1, n, k), 0.0) - 1e-12, "P_A >= payoff");
    check(earlyExerciseNodes(100, 0.1, -0.1, 0.02, 4, call(100)).empty(), "never exercise a call early");
    check(near(optionTree(100, 0.1, -0.1, 0.02, 4, call(100), true)[0][0],
               optionTree(100, 0.1, -0.1, 0.02, 4, call(100))[0][0]),
          "C_A = C_E");

    // 8.3
    check(near(normCdf(0), 0.5, 1e-14), "N(0)");
    check(near(normCdf(1.959963984540054), 0.975, 1e-12), "N(1.96)");
    check(near(normCdf(-8) + normCdf(8), 1, 1e-14), "N symmetry");
    const double C = bsCall(100, 100, 0.05, 0.2, 1), P = bsPut(100, 100, 0.05, 0.2, 1);
    check(near(C, 10.450583572185565, 1e-10), "BS call");
    check(near(C - P, 100 - 100 * std::exp(-0.05)), "BS parity");
    const double h = 1e-4;
    const double fd = (bsCall(100 + h, 100, 0.05, 0.2, 1) - bsCall(100 - h, 100, 0.05, 0.2, 1)) / (2 * h);
    check(near(bsCallDelta(100, 100, 0.05, 0.2, 1), fd, 1e-6), "delta");
    check(near(crrTreePrice(100, 100, 0.05, 0.2, 1, 2000, true), C, 1e-2), "CRR converges to BS");

    if (failures == 0) std::printf("All ch08 tests passed\n");
    return failures == 0 ? 0 : 1;
}
