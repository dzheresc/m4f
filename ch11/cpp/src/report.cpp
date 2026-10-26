#include "m4f/report.hpp"
#include "m4f/derivatives.hpp"
#include "m4f/rate_tree.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace m4f::ch11 {
namespace {

std::string f(double v) { return std::format("{:.4f}", v + 0.0); }

std::string fl(const std::vector<double>& vs) {
    std::string s = "[";
    for (std::size_t i = 0; i < vs.size(); ++i) s += (i ? ", " : "") + f(vs[i]);
    return s + "]";
}

std::vector<double> rev(std::vector<double> v) {
    std::reverse(v.begin(), v.end());
    return v;
}

}  // namespace

void printReport(std::ostream& os) {
    os << "Chapter 11: Stochastic Interest Rates (illustrative numbers, book formulas)\n";

    const int N = 4;
    const double tau = 1.0;
    const Lattice rates = rateLattice(0.05, 0.01, -0.01, N);
    const Lattice probs = constantProbs(0.5, N);

    os << "\n== 11.1 Binomial tree model: r(n,k) = 0.05 + 0.01 k - 0.01 (n-k), tau=1, N=4 ==\n";
    for (int n = 0; n <= N; ++n) {
        std::vector<double> one;
        for (double r : rev(rates[n])) one.push_back(oneStepPrice(r, tau));
        os << std::format("n={}: r={} B(n,n+1)={}\n", n, fl(rev(rates[n])), fl(one));
    }
    for (const char* path : {"uuuu", "udud", "dddd"})
        os << std::format("Money market account along {}: A={}\n", path, fl(moneyMarket(rates, tau, path)));

    os << "\n== 11.2 Arbitrage pricing of bonds with p*=0.5 ==\n";
    const std::vector<double> B0 = bondPrices(rates, probs, tau, N);
    os << "B(0,N) for N=1..4: " << fl(B0) << "\n";
    std::vector<double> ys;
    for (int m = 0; m < N; ++m) ys.push_back(yieldRate(B0[m], 0, m + 1, tau));
    os << "Term structure y(0,N): " << fl(ys) << "\n";
    const Lattice B4 = bondLattice(rates, probs, tau, N);
    for (int n = 0; n <= N; ++n) os << std::format("B(n,4) at n={}: {}\n", n, fl(rev(B4[n])));
    const Lattice B3 = bondLattice(rates, probs, tau, 3);
    const Lattice B2 = bondLattice(rates, probs, tau, 2);
    const std::vector<std::pair<int, const Lattice*>> lattices = {{2, &B2}, {3, &B3}, {4, &B4}};
    std::vector<double> yu, yd, ret;
    for (const auto& [m, B] : lattices) {
        yu.push_back(yieldRate((*B)[1][1], 1, m, tau));
        yd.push_back(yieldRate((*B)[1][0], 1, m, tau));
        ret.push_back((0.5 * (*B)[1][1] + 0.5 * (*B)[1][0]) / (*B)[0][0]);
    }
    os << "Term structure after one step: up node y(1,N)=" << fl(yu) << " down node y(1,N)=" << fl(yd) << "\n";
    os << "Risk-neutral one-step return E*[B(1,N)]/B(0,N) for N=2,3,4: " << fl(ret)
       << std::format(" = e^(r(0) tau) = {}\n", f(std::exp(0.05 * tau)));

    os << "\n== 11.2.1 Risk-neutral probabilities implied by bond prices ==\n";
    for (const auto& [m, B] : {std::pair<int, const Lattice*>{4, &B4}, {3, &B3}, {2, &B2}}) {
        const Lattice P = impliedProbs(*B, rates, tau);
        std::string row = std::format("from B(n,{}):", m);
        for (std::size_t n = 0; n < P.size(); ++n) row += std::format(" n={}:{}", n, fl(rev(P[n])));
        os << row << "\n";
    }
    const Lattice probs6 = constantProbs(0.6, N);
    const Lattice B4_6 = bondLattice(rates, probs6, tau, N);
    os << std::format("Bonds priced with p*=0.6: B(0,4)={} (vs {}); implied p* at (0,0): {}\n", f(B4_6[0][0]), f(B4[0][0]),
                      f(impliedProb(B4_6, rates, tau, 0, 0)));
    Lattice wrong = B2;
    wrong[0][0] = B2[0][0] - 0.01;
    os << std::format("A two-year bond quoted at {} instead of {} implies p*={} at (0,0), inconsistent with the other bonds: arbitrage\n",
                      f(wrong[0][0]), f(B2[0][0]), f(impliedProb(wrong, rates, tau, 0, 0)));

    os << "\n== 11.3.1 Options on the 4-year bond, expiry n=2, p*=0.5 ==\n";
    const double X = 0.90;
    for (bool isCall : {true, false}) {
        const Lattice V = bondOption(rates, probs, tau, N, 2, X, isCall);
        os << std::format("European {} X={}: payoff at n=2 {}, price {}\n", isCall ? "call" : "put", f(X), fl(rev(V[2])), f(V[0][0]));
    }
    const Lattice VA = bondOption(rates, probs, tau, N, 2, X, false, true);
    std::vector<double> ex;
    for (double b : rev(B4[1])) ex.push_back(std::max(X - b, 0.0));
    os << std::format("American put X={}: price {} = immediate exercise X - B(0,4) = {}; values at n=1 {} = exercise values {}\n",
                      f(X), f(VA[0][0]), f(X - B4[0][0]), fl(rev(VA[1])), fl(ex));
    os << std::format("American call X=0.85 on the 4-year bond, expiry n=3: {} vs European {}\n",
                      f(bondOption(rates, probs, tau, N, 3, 0.85, true, true)[0][0]), f(bondOption(rates, probs, tau, N, 3, 0.85, true)[0][0]));
    os << "Call on the 2-year bond expiring at n=1, X=0.95: " << f(bondOption(rates, probs, tau, 2, 1, 0.95, true)[0][0]) << "\n";

    os << "\n== 11.3.2 Swaps: fixed K against floating L(n,k) = e^(r tau) - 1, 4 annual payments ==\n";
    for (int n = 0; n < N; ++n) {
        std::vector<double> L;
        for (int k = n; k >= 0; --k) L.push_back(floatingRate(rates, tau, n, k));
        os << std::format("n={}: L={}\n", n, fl(L));
    }
    const double K = swapRate(B0);
    os << std::format("Swap rate K = (1 - B(0,4)) / sum B(0,n) = {}\n", f(K));
    const Lattice S = payerSwap(rates, probs, tau, N, K);
    os << std::format("Payer swap value at K: {}; at n=1: {}\n", f(S[0][0]), fl(rev(S[1])));
    for (double Kx : {0.04, 0.05, 0.06}) os << std::format("Payer swap at K={}: {}\n", f(Kx), f(payerSwap(rates, probs, tau, N, Kx)[0][0]));

    os << "\n== 11.3.3 Caps and floors on the same floating rate ==\n";
    for (double Kx : {0.04, K, 0.06}) {
        const double c = cap(rates, probs, tau, N, Kx)[0][0], fo = floor(rates, probs, tau, N, Kx)[0][0],
                     s = payerSwap(rates, probs, tau, N, Kx)[0][0];
        os << std::format("K={}: cap={} floor={} cap-floor={} = payer swap {}\n", f(Kx), f(c), f(fo), f(c - fo), f(s));
    }
    std::vector<double> cs;
    double sum = 0.0;
    for (int n = 0; n < N; ++n) { cs.push_back(caplet(rates, probs, tau, n, 0.05)); sum += cs.back(); }
    os << "Caplets at K=0.05 by fixing date n=0..3: " << fl(cs)
       << std::format(", sum {} = cap {}\n", f(sum), f(cap(rates, probs, tau, N, 0.05)[0][0]));
}

}  // namespace m4f::ch11
