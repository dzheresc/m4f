// Sections 11.1-11.2: the binomial tree model of the short rate.
//
// Time steps of length tau; node (n,k) is reached after n steps of which k
// were "up". A Lattice holds one value per node: L[n][k], k = 0..n. The short
// rate r(n,k) applies over [n, n+1], so the one-step bond is
// B(n,n+1;k) = e^(-r(n,k) tau) and the money market account grows by
// e^(r tau) each step. Risk-neutral probabilities p*(n,k) of an up move may
// vary by node.
#ifndef M4F_CH11_RATE_TREE_H_
#define M4F_CH11_RATE_TREE_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace m4f::ch11 {

using Lattice = std::vector<std::vector<double>>;
// A value attached to node (n, k).
using NodeFn = std::function<double(int, int)>;

// Additive tree: r(n,k) = r0 + k up + (n-k) down (down is negative for a
// fall).
inline Lattice RateLattice(double r0, double up, double down, int num_steps) {
  Lattice lattice(num_steps + 1);
  for (int n = 0; n <= num_steps; ++n) {
    std::vector<double>& row = lattice[n];
    row.resize(n + 1);
    for (int k = 0; k <= n; ++k) row[k] = r0 + k * up + (n - k) * down;
  }
  return lattice;
}

// The same up probability p at every node of levels 0..num_steps-1.
inline Lattice ConstantProbs(double p, int num_steps) {
  Lattice lattice(num_steps);
  for (int n = 0; n < num_steps; ++n) lattice[n].assign(n + 1, p);
  return lattice;
}

// B(n,n+1) = e^(-r tau).
inline double OneStepPrice(double r, double tau) { return std::exp(-r * tau); }

// A(0..len(path)) along a path of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k) tau).
inline std::vector<double> MoneyMarket(const Lattice& rates, double tau,
                                       const std::string& path) {
  std::vector<double> account = {1.0};
  int k = 0;
  for (std::size_t n = 0; n < path.size(); ++n) {
    account.push_back(account.back() * std::exp(rates[n][k] * tau));
    if (path[n] == 'u') ++k;
  }
  return account;
}

// Backward induction from level N = terminal.size()-1 down to 0:
// V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
// with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
// is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise).
inline Lattice DiscountBack(const Lattice& rates, const Lattice& probs,
                            double tau, const std::vector<double>& terminal,
                            const NodeFn& payment = nullptr,
                            const NodeFn& early = nullptr) {
  const int num_steps = static_cast<int>(terminal.size()) - 1;
  Lattice values(num_steps + 1);
  values[num_steps] = terminal;
  for (int n = num_steps - 1; n >= 0; --n) {
    const std::vector<double>& next = values[n + 1];
    std::vector<double>& row = values[n];
    row.reserve(n + 1);
    for (int k = 0; k <= n; ++k) {
      const double p = probs[n][k];
      double cont = p * next[k + 1] + (1 - p) * next[k];
      if (payment) cont += payment(n, k);
      double v = OneStepPrice(rates[n][k], tau) * cont;
      if (early) v = std::max(v, early(n, k));
      row.push_back(v);
    }
  }
  return values;
}

// 11.2: B(n,N;k) for all nodes, from B(N,N) = 1.
inline Lattice BondLattice(const Lattice& rates, const Lattice& probs,
                           double tau, int maturity) {
  return DiscountBack(rates, probs, tau,
                      std::vector<double>(maturity + 1, 1.0));
}

// B(0,1), ..., B(0,N), by forward induction over the Arrow-Debreu state
// prices Q(n,k), the time-0 value of 1 paid at node (n,k):
// B(0,n+1) = sum_k Q(n,k) e^(-r(n,k) tau), and Q(n+1,.) receives
// Q(n,k) e^(-r(n,k) tau) times p* (up) or 1 - p* (down). One O(N^2) pass
// instead of a backward induction per maturity; equal to BondLattice()[0][0]
// up to rounding.
inline std::vector<double> BondPrices(const Lattice& rates,
                                      const Lattice& probs, double tau,
                                      int num_steps) {
  std::vector<double> out;
  out.reserve(num_steps);
  std::vector<double> q = {1.0};
  std::vector<double> next;
  for (int n = 0; n < num_steps; ++n) {
    next.assign(n + 2, 0.0);
    double bond = 0.0;
    for (int k = 0; k <= n; ++k) {
      const double d = q[k] * OneStepPrice(rates[n][k], tau);
      bond += d;
      const double p = probs[n][k];
      next[k + 1] += p * d;
      next[k] += (1 - p) * d;
    }
    out.push_back(bond);
    q.swap(next);
  }
  return out;
}

// y(n,N) = -ln B(n,N) / ((N-n) tau).
inline double YieldRate(double bond_price, int n, int maturity, double tau) {
  return -std::log(bond_price) / ((maturity - n) * tau);
}

// 11.2.1: the p*(n,k) that prices the bond with lattice `bond` at node (n,k):
// B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k).
inline double ImpliedProb(const Lattice& bond, const Lattice& rates, double tau,
                          int n, int k) {
  return (bond[n][k] * std::exp(rates[n][k] * tau) - bond[n + 1][k]) /
         (bond[n + 1][k + 1] - bond[n + 1][k]);
}

// ImpliedProb() at every node of levels 0..N-2 of `bond`.
inline Lattice ImpliedProbs(const Lattice& bond, const Lattice& rates,
                            double tau) {
  const int maturity = static_cast<int>(bond.size()) - 1;
  Lattice probs(maturity - 1);
  for (int n = 0; n < maturity - 1; ++n) {
    for (int k = 0; k <= n; ++k) {
      probs[n].push_back(ImpliedProb(bond, rates, tau, n, k));
    }
  }
  return probs;
}

}  // namespace m4f::ch11

#endif  // M4F_CH11_RATE_TREE_H_
