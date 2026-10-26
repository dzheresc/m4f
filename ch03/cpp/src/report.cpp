#include "m4f/report.hpp"
#include "m4f/binomial_tree.hpp"
#include "m4f/other_models.hpp"
#include "m4f/returns.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <ostream>

namespace m4f::ch03 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 3: Risky Assets (illustrative numbers, book formulas)\n";

    os << "\n== 3.1.1 Return: K(n) = (S(n) - S(n-1)) / S(n-1), k(n) = ln(S(n) / S(n-1)) ==\n";
    const std::vector<double> path{45.0, 48.0, 47.5, 50.0};
    const auto Ks = returns(path), ks = logReturns(path);
    for (std::size_t n = 1; n < path.size(); ++n)
        os << std::format("n={}: S={} K={} k={}\n", n, f(path[n]), f(Ks[n - 1]), f(ks[n - 1]));
    os << std::format("K(0,3) from S: {}, from (1+K(1))(1+K(2))(1+K(3))-1: {}\n",
                      f(oneStepReturn(path[0], path[3])), f(compoundReturn(Ks)));
    os << std::format("k(0,3) from S: {}, k(1)+k(2)+k(3): {}\n", f(logReturn(path[0], path[3])),
                      f(ks[0] + ks[1] + ks[2]));
    os << std::format("S(0)=45, S(1)=48 with dividend 2: K(1)={}\n", f(oneStepReturn(45.0, 48.0, 2.0)));

    os << "\n== 3.1.2 Expected return ==\n";
    const std::vector<double> S1s{52.0, 48.0, 40.0}, probs{0.25, 0.5, 0.25};
    std::vector<double> K1s;
    for (double s : S1s) K1s.push_back(oneStepReturn(45.0, s));
    const double EK = expectation(K1s, probs);
    os << std::format("S(0)=45, S(1) in {{52, 48, 40}} with p={{0.25, 0.5, 0.25}}: E(K)={} sd(K)={}\n",
                      f(EK), f(std::sqrt(variance(K1s, probs))));
    os << std::format("E(S(1)) = (1 + E(K)) S(0) = {}, directly {}\n", f((1 + EK) * 45.0),
                      f(expectation(S1s, probs)));
    os << std::format("Independent steps, E(K(1))=0.10, E(K(2))=0.05: E(K(0,2)) = {}\n",
                      f(compoundReturn({0.10, 0.05})));

    os << "\n== 3.2 Binomial tree model: S(0)=100 u=0.10 d=-0.05 r=0.03 p=0.6 ==\n";
    const BinomialTree tree{100.0, 0.10, -0.05, 0.03};
    for (int n = 0; n < 4; ++n) {
        std::string nodes;
        for (int k = n; k >= 0; --k) {
            if (k != n) nodes += ' ';
            nodes += std::format("{}({})", f(tree.price(n, k)), f(binomialProb(n, k, 0.6)));
        }
        os << std::format("n={}: {}\n", n, nodes);
    }
    os << std::format("E(K(n))={} E(S(3))={} (1+E(K))^3 S(0)={}\n", f(tree.expectedReturn(0.6)),
                      f(tree.expectedPrice(3, 0.6)), f(std::pow(1 + tree.expectedReturn(0.6), 3) * 100.0));

    os << "\n== 3.2.1 Risk-neutral probability: p* = (r - d) / (u - d) ==\n";
    const double q = tree.riskNeutralProb();
    os << std::format("d < r < u: {}, p*={}, E*(K)={}\n", tree.isArbitrageFree() ? "yes" : "no", f(q),
                      f(tree.expectedReturn(q)));
    os << std::format("E*(S(1))={} (1+r)S(0)={}; E*(S(3))={} (1+r)^3 S(0)={}\n",
                      f(tree.expectedPrice(1, q)), f(1.03 * 100.0), f(tree.expectedPrice(3, q)),
                      f(std::pow(1.03, 3) * 100.0));

    os << "\n== 3.2.2 Martingale property: E*(S~(n+1) | S(n)) = S~(n) ==\n";
    double worst = 0.0;
    for (int n = 0; n < 3; ++n)
        for (int k = 0; k <= n; ++k)
            worst = std::max(worst, std::abs(tree.conditionalDiscounted(n, k) - tree.discountedPrice(n, k)));
    for (int k = 1; k >= 0; --k)
        os << std::format("node (1,{}): S~(1)={} E*(S~(2)|.)={}\n", k, f(tree.discountedPrice(1, k)),
                          f(tree.conditionalDiscounted(1, k)));
    os << std::format("Largest deviation over all nodes n<3: {}\n", f(worst));

    os << "\n== 3.3.1 Trinomial tree: u=0.10 m=0 d=-0.10 r=0.02, S(0)=100 ==\n";
    const TrinomialModel tri{0.10, 0.0, -0.10, 0.02};
    os << std::format("Risk-neutral probabilities exist for 0 < p_m < {}\n", f(tri.maxMiddleProb()));
    for (double pm : {0.2, 0.4, 0.6}) {
        const auto [pu, pmid, pd] = tri.riskNeutralProbs(pm);
        const double call = pu * 10.0 / 1.02;
        os << std::format("p_m={}: p_u={} p_d={} E*(K)={} call(K=100) price={}\n", f(pmid), f(pu), f(pd),
                          f(pu * 0.10 + pm * 0.0 + pd * -0.10), f(call));
    }

    os << "\n== 3.3.2 Continuous-time limit: S(0)=100 m=0.05 sigma=0.2 T=1 ==\n";
    for (int N : {1, 10, 100, 1000, 10000})
        os << std::format("N={}: E(S(T))={} P(S(T)>110)={}\n", N,
                          f(binomialExpectedPrice(100.0, 0.05, 0.2, 1.0, N)),
                          f(binomialProbAbove(100.0, 0.05, 0.2, 1.0, N, 110.0)));
    os << std::format("Limit: E(S(T))={} P(S(T)>110)={}\n", f(lognormalExpectedPrice(100.0, 0.05, 0.2, 1.0)),
                      f(lognormalProbAbove(100.0, 0.05, 0.2, 1.0, 110.0)));
}

}  // namespace m4f::ch03
