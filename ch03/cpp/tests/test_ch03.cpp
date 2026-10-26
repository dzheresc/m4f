#include "m4f/binomial_tree.hpp"
#include "m4f/other_models.hpp"
#include "m4f/returns.hpp"

#include <cmath>
#include <cstdio>

using namespace m4f::ch03;

static int failures = 0;
static void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

int main() {
    // Report values
    check(near(BinomialTree{100, 0.10, -0.05, 0.03}.expectedPrice(3, 0.6), 112.4864), "E(S(3))");
    check(near(BinomialTree{100, 0.10, -0.05, 0.03}.riskNeutralProb(), 8.0 / 15), "p*");
    check(near(TrinomialModel{0.10, 0.0, -0.10, 0.02}.maxMiddleProb(), 0.8), "p_m bound");
    check(near(lognormalExpectedPrice(100, 0.05, 0.2, 1), 100 * std::exp(0.07)), "lognormal mean");

    // 3.1 returns
    const std::vector<double> path{45.0, 48.0, 47.5, 50.0};
    check(near(compoundReturn(returns(path)), oneStepReturn(45, 50)), "compound return");
    const auto ks = logReturns(path);
    check(near(ks[0] + ks[1] + ks[2], logReturn(45, 50)), "log returns add");
    check(near(expectation({1, 3}, {0.5, 0.5}), 2), "expectation");
    check(near(variance({1, 3}, {0.5, 0.5}), 1), "variance");

    // 3.2 binomial tree
    double total = 0;
    for (int k = 0; k <= 5; ++k) total += binomialProb(5, k, 0.3);
    check(near(total, 1), "binomial probabilities sum to 1");
    const BinomialTree tree{60, 0.2, -0.1, 0.05};
    const double q = tree.riskNeutralProb();
    check(tree.isArbitrageFree(), "d < r < u");
    check(near(tree.expectedReturn(q), 0.05), "E*(K) = r");
    for (int n = 0; n < 4; ++n) {
        check(near(tree.expectedPrice(n, q), 60 * std::pow(1.05, n)), "E*(S(n))");
        check(near(tree.expectedPrice(n, 0.4), 60 * std::pow(1 + tree.expectedReturn(0.4), n)), "E(S(n))");
        for (int k = 0; k <= n; ++k)
            check(near(tree.conditionalDiscounted(n, k), tree.discountedPrice(n, k)), "martingale");
    }
    check(!BinomialTree{60, 0.2, 0.1, 0.05}.isArbitrageFree(), "arbitrage when r < d");

    // 3.3.1 trinomial
    const TrinomialModel tri{0.10, 0.0, -0.10, 0.02};
    for (double pm : {0.1, 0.5, 0.79}) {
        const auto p = tri.riskNeutralProbs(pm);
        check(near(p.pu + p.pm + p.pd, 1), "trinomial sum");
        check(near(p.pu * 0.10 + p.pd * -0.10, 0.02), "trinomial E*(K) = r");
        check(p.pu > 0 && p.pd > 0, "trinomial positive");
    }
    check(tri.riskNeutralProbs(0.81).pd < 0, "p_m above bound");

    // 3.3.2 continuous-time limit
    check(near(normCdf(0), 0.5, 1e-7), "Phi(0)");
    check(near(normCdf(1.96), 0.9750021, 1e-6), "Phi(1.96)");
    check(near(normCdf(-1) + normCdf(1), 1), "Phi symmetry");
    check(near(binomialExpectedPrice(100, 0.05, 0.2, 1, 100000), lognormalExpectedPrice(100, 0.05, 0.2, 1), 1e-4),
          "E(S(T)) limit");
    check(near(binomialProbAbove(100, 0.05, 0.2, 1, 20000, 110), lognormalProbAbove(100, 0.05, 0.2, 1, 110), 1e-2),
          "P(S(T)>K) limit");
    check(near(binomialProbAbove(100, 0.0, 0.2, 1, 7, 1e-9), 1), "P(S(T)>0) = 1");

    if (failures == 0) std::printf("All ch03 tests passed\n");
    return failures == 0 ? 0 : 1;
}
