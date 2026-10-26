#include "m4f/report.hpp"
#include "m4f/binomial_pricing.hpp"
#include "m4f/black_scholes.hpp"

#include <algorithm>
#include <format>
#include <ostream>

namespace m4f::ch08 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const std::vector<double>& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

Payoff call(double X) { return [X](double s) { return std::max(s - X, 0.0); }; }
Payoff put(double X) { return [X](double s) { return std::max(X - s, 0.0); }; }

std::vector<double> reversed(std::vector<double> v) {
    std::reverse(v.begin(), v.end());
    return v;
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 8: Option Pricing (illustrative numbers, book formulas)\n";

    os << "\n== 8.1.1 One step: S(0)=100 u=0.2 d=-0.1 r=0.1, call X=100 ==\n";
    const Replication rep = oneStepReplication(100.0, 0.2, -0.1, 0.1, 20.0, 0.0);
    const double q = riskNeutralProb(0.2, -0.1, 0.1);
    os << std::format("Replicate with x={} shares, y={} in the money market: C(0)={}\n", f(rep.x), f(rep.y),
                      f(rep.price));
    os << std::format("Risk-neutral: p*={}, C(0) = E*(C(1))/(1+r) = {}\n", f(q), f((q * 20.0 + (1 - q) * 0.0) / 1.1));

    os << "\n== 8.1.2 Two steps: same tree, call X=100 ==\n";
    const Tree V = optionTree(100.0, 0.2, -0.1, 0.1, 2, call(100.0));
    for (int n = 0; n < 3; ++n) {
        std::vector<double> S;
        for (int k = n; k >= 0; --k) S.push_back(stockPrice(100.0, 0.2, -0.1, n, k));
        os << std::format("n={}: S={} C={}\n", n, fl(S), fl(reversed(V[n])));
    }
    for (const auto& [n, k] : std::vector<std::pair<int, int>>{{0, 0}, {1, 1}, {1, 0}}) {
        const auto [xs, ys] = replicatingPosition(100.0, 0.2, -0.1, 0.1, V, n, k);
        os << std::format("node ({},{}): x={} y={}\n", n, k, f(xs), f(ys));
    }

    os << "\n== 8.1.3 General N-step model: N=5, X=100 ==\n";
    for (const auto& [name, pay] : std::vector<std::pair<const char*, Payoff>>{{"call", call(100.0)}, {"put", put(100.0)}})
        os << std::format("{}: backward induction={} sum formula={}\n", name,
                          f(optionTree(100.0, 0.2, -0.1, 0.1, 5, pay)[0][0]),
                          f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, pay)));
    const Payoff spread = [](double s) { return std::min(std::max(s - 90.0, 0.0), 30.0); };  // bull spread 90/120
    os << std::format("bull spread 90/120: {}\n", f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, spread)));

    os << "\n== 8.1.4 Cox-Ross-Rubinstein formula ==\n";
    for (double X : {80.0, 100.0, 150.0}) {
        const CrrCall c = crrCall(100.0, X, 0.2, -0.1, 0.1, 5);
        os << std::format("N=5 X={}: m={} C={} sum formula={}\n", f(X), c.m, f(c.price),
                          f(europeanPriceSum(100.0, 0.2, -0.1, 0.1, 5, call(X))));
    }

    os << "\n== 8.2 American options: S(0)=100 u=0.1 d=-0.1 r=0.02 N=4, put X=100 ==\n";
    const Tree VA = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0), true);
    const Tree VE = optionTree(100.0, 0.1, -0.1, 0.02, 4, put(100.0));
    os << std::format("P_A={} P_E={}\n", f(VA[0][0]), f(VE[0][0]));
    for (int n = 0; n < 5; ++n) os << std::format("n={}: P_A={}\n", n, fl(reversed(VA[n])));
    std::string nodes;
    for (const auto& [n, k] : earlyExerciseNodes(100.0, 0.1, -0.1, 0.02, 4, put(100.0))) {
        if (!nodes.empty()) nodes += ", ";
        nodes += std::format("({},{}) S={}", n, k, f(stockPrice(100.0, 0.1, -0.1, n, k)));
    }
    os << "Exercise early at (n,k): " << nodes << "\n";

    os << "\n== 8.3 Black-Scholes formula: S=100 X=100 r=0.05 sigma=0.2 T=1 ==\n";
    os << std::format("C={} P={} delta={}\n", f(bsCall(100.0, 100.0, 0.05, 0.2, 1.0)),
                      f(bsPut(100.0, 100.0, 0.05, 0.2, 1.0)), f(bsCallDelta(100.0, 100.0, 0.05, 0.2, 1.0)));
    for (int N : {10, 50, 100, 500, 1000})
        os << std::format("CRR N={}: C={} P={} P_A={}\n", N, f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, true)),
                          f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, false)),
                          f(crrTreePrice(100.0, 100.0, 0.05, 0.2, 1.0, N, false, true)));
}

}  // namespace m4f::ch08
