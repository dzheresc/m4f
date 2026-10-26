"""Section 3.2, the binomial tree model: S(n) = S(0) (1+u)^k (1+d)^(n-k) after
k up moves in n steps, with the money market growing at r per step.
"""
from __future__ import annotations


def binomial_prob(n: int, k: int, p: float) -> float:
    """C(n, k) p^k (1-p)^(n-k)."""
    c = 1.0
    for i in range(k):
        c = c * (n - i) / (i + 1)
    return c * p ** k * (1 - p) ** (n - k)


class BinomialTree:
    def __init__(self, S0: float, u: float, d: float, r: float) -> None:
        self.S0, self.u, self.d, self.r = S0, u, d, r

    def price(self, n: int, k: int) -> float:
        """S(n) after k up moves out of n."""
        return self.S0 * (1 + self.u) ** k * (1 + self.d) ** (n - k)

    def expected_return(self, p: float) -> float:
        """E(K(n)) = p u + (1-p) d, the same for every step."""
        return p * self.u + (1 - p) * self.d

    def expected_price(self, n: int, p: float) -> float:
        """E(S(n)) as a sum over the n+1 nodes at step n."""
        s = 0.0
        for k in range(n + 1):
            s += self.price(n, k) * binomial_prob(n, k, p)
        return s

    # 3.2.1 Risk-neutral probability
    def is_arbitrage_free(self) -> bool:
        return self.d < self.r < self.u

    def risk_neutral_prob(self) -> float:
        """p* = (r - d) / (u - d), so that E*(K(n)) = r."""
        return (self.r - self.d) / (self.u - self.d)

    # 3.2.2 Martingale property
    def discounted_price(self, n: int, k: int) -> float:
        """S~(n) = (1+r)^-n S(n)."""
        return self.price(n, k) * (1 + self.r) ** -n

    def conditional_discounted(self, n: int, k: int) -> float:
        """E*(S~(n+1) | S(n) at node (n, k)); equals S~(n) under p*."""
        q = self.risk_neutral_prob()
        return q * self.discounted_price(n + 1, k + 1) + (1 - q) * self.discounted_price(n + 1, k)
