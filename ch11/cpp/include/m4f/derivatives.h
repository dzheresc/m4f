// Section 11.3: interest rate derivatives on the binomial tree.
//
// Floating payments are the simple one-period rate fixed at (n,k) and paid at
// n+1: L(n,k) = 1/B(n,n+1;k) - 1 = e^(r(n,k) tau) - 1, on a notional of 1.
#ifndef M4F_CH11_DERIVATIVES_H_
#define M4F_CH11_DERIVATIVES_H_

#include <algorithm>
#include <vector>

#include "m4f/rate_tree.h"

namespace m4f::ch11 {

// L(n,k) = 1/B(n,n+1;k) - 1.
inline double FloatingRate(const Lattice& rates, double tau, int n, int k) {
  return 1 / OneStepPrice(rates[n][k], tau) - 1;
}

// 11.3.1 Options on a zero-coupon bond maturing at `maturity`, expiring at
// `expiry` <= maturity.
inline Lattice BondOption(const Lattice& rates, const Lattice& probs,
                          double tau, int maturity, int expiry, double strike,
                          bool is_call, bool american = false) {
  const Lattice bond = BondLattice(rates, probs, tau, maturity);
  const auto payoff = [strike, is_call](double b) {
    return is_call ? std::max(b - strike, 0.0) : std::max(strike - b, 0.0);
  };
  std::vector<double> terminal;
  for (int k = 0; k <= expiry; ++k) terminal.push_back(payoff(bond[expiry][k]));
  const NodeFn early =
      american
          ? NodeFn([&bond, payoff](int n, int k) { return payoff(bond[n][k]); })
          : NodeFn();
  return DiscountBack(rates, probs, tau, terminal, nullptr, early);
}

// 11.3.2 Swaps: fixed K against floating L(n,k), N payments at 1..N on
// notional 1. The fixed rate making the swap worthless:
// K sum B(0,n) = 1 - B(0,N).
inline double SwapRate(const std::vector<double>& bond_prices) {
  double s = 0.0;
  for (double b : bond_prices) s += b;
  return (1 - bond_prices.back()) / s;
}

// Value lattice of a stream paying payment(n,k) at n+1 for n = 0..N-1.
inline Lattice PricePayments(const Lattice& rates, const Lattice& probs,
                             double tau, int num_steps, const NodeFn& payment) {
  return DiscountBack(rates, probs, tau,
                      std::vector<double>(num_steps + 1, 0.0), payment);
}

// Receive floating, pay fixed `fixed_rate`.
inline Lattice PayerSwap(const Lattice& rates, const Lattice& probs, double tau,
                         int num_steps, double fixed_rate) {
  return PricePayments(rates, probs, tau, num_steps,
                       [&rates, tau, fixed_rate](int n, int k) {
                         return FloatingRate(rates, tau, n, k) - fixed_rate;
                       });
}

// 11.3.3 Caps and floors: caplet pays max(L(n,k) - K, 0), floorlet
// max(K - L(n,k), 0).
inline Lattice Cap(const Lattice& rates, const Lattice& probs, double tau,
                   int num_steps, double strike) {
  return PricePayments(
      rates, probs, tau, num_steps, [&rates, tau, strike](int n, int k) {
        return std::max(FloatingRate(rates, tau, n, k) - strike, 0.0);
      });
}

inline Lattice Floor(const Lattice& rates, const Lattice& probs, double tau,
                     int num_steps, double strike) {
  return PricePayments(
      rates, probs, tau, num_steps, [&rates, tau, strike](int n, int k) {
        return std::max(strike - FloatingRate(rates, tau, n, k), 0.0);
      });
}

// Value at 0 of the single caplet fixed at `fixing` and paid at fixing + 1.
// Only level `fixing` pays, so this rolls one row back from there instead of
// building the whole PricePayments() lattice; the result is the same.
inline double Caplet(const Lattice& rates, const Lattice& probs, double tau,
                     int fixing, double strike) {
  std::vector<double> v(fixing + 1);
  for (int k = 0; k <= fixing; ++k) {
    v[k] = OneStepPrice(rates[fixing][k], tau) *
           std::max(FloatingRate(rates, tau, fixing, k) - strike, 0.0);
  }
  for (int n = fixing - 1; n >= 0; --n) {
    for (int k = 0; k <= n; ++k) {
      const double p = probs[n][k];
      v[k] = OneStepPrice(rates[n][k], tau) * (p * v[k + 1] + (1 - p) * v[k]);
    }
  }
  return v[0];
}

}  // namespace m4f::ch11

#endif  // M4F_CH11_DERIVATIVES_H_
