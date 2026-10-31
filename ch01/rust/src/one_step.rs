//! The one-step binomial market.
//!
//! Two assets, two time instants t = 0, 1:
//!
//! ```text
//! risk-free bond  A(0) -> A(1)                     (return r)
//! risky stock     S(0) -> S^u(1) with prob. p      (return u)
//!                       S^d(1) with prob. 1 - p    (return d)
//! ```
//!
//! A portfolio (x, y) holds x shares and y bonds; negative = short /
//! borrowed.
//!
//! Names follow the book's notation, lower-cased for the style guide:
//! `a0` = A(0), `a1` = A(1), `s0` = S(0), `s_up` = S^u(1), `s_down` = S^d(1).

/// The state of the world at t = 1.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum State {
    /// The stock went up to S^u(1).
    Up,
    /// The stock went down to S^d(1).
    Down,
}

/// A portfolio of `x` shares and `y` bonds.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Portfolio {
    /// Shares of stock.
    pub x: f64,
    /// Units of bond.
    pub y: f64,
}

/// The one-step binomial market of Chapter 1.
#[derive(Clone, Copy, Debug)]
pub struct OneStepMarket {
    a0: f64,
    a1: f64,
    s0: f64,
    s_up: f64,
    s_down: f64,
    p: f64,
}

impl OneStepMarket {
    /// Builds the market: bond A(0) = `a0` -> A(1) = `a1`, stock S(0) = `s0`
    /// -> S^u(1) = `s_up` with probability `p` or S^d(1) = `s_down`.
    ///
    /// # Panics
    ///
    /// Unless all prices are positive, `s_up > s_down` and `0 < p < 1`.
    pub fn new(a0: f64, a1: f64, s0: f64, s_up: f64, s_down: f64, p: f64) -> Self {
        assert!(
            a0 > 0.0 && a1 > 0.0 && s0 > 0.0 && s_down > 0.0 && s_up > s_down,
            "require positive prices and S^u(1) > S^d(1)"
        );
        assert!(p > 0.0 && p < 1.0, "require 0 < p < 1");
        Self {
            a0,
            a1,
            s0,
            s_up,
            s_down,
            p,
        }
    }

    /// A(0), the bond price at t = 0.
    pub fn a0(&self) -> f64 {
        self.a0
    }
    /// A(1), the bond price at t = 1.
    pub fn a1(&self) -> f64 {
        self.a1
    }
    /// S(0), the stock price at t = 0.
    pub fn s0(&self) -> f64 {
        self.s0
    }
    /// S(1) in state `s`.
    pub fn s1(&self, s: State) -> f64 {
        match s {
            State::Up => self.s_up,
            State::Down => self.s_down,
        }
    }
    /// The probability p of the up state.
    pub fn p(&self) -> f64 {
        self.p
    }

    /// Return K_A = r of the bond.
    pub fn r(&self) -> f64 {
        (self.a1 - self.a0) / self.a0
    }
    /// Return u of the stock in the up state.
    pub fn u(&self) -> f64 {
        (self.s_up - self.s0) / self.s0
    }
    /// Return d of the stock in the down state.
    pub fn d(&self) -> f64 {
        (self.s_down - self.s0) / self.s0
    }

    /// Theorem: no arbitrage <=> d < r < u.
    pub fn is_arbitrage_free(&self) -> bool {
        self.d() < self.r() && self.r() < self.u()
    }

    /// Risk-neutral probability p* = (r - d) / (u - d); makes
    /// E*[S(1)] = (1 + r) S(0).
    pub fn risk_neutral_prob(&self) -> f64 {
        (self.r() - self.d()) / (self.u() - self.d())
    }

    /// Portfolio investing `budget` with fraction `w` in stock, the rest in
    /// bonds.
    pub fn split(&self, budget: f64, w: f64) -> Portfolio {
        Portfolio {
            x: w * budget / self.s0,
            y: (1.0 - w) * budget / self.a0,
        }
    }

    /// V(0), the portfolio value at t = 0.
    pub fn value0(&self, portfolio: &Portfolio) -> f64 {
        portfolio.x * self.s0 + portfolio.y * self.a0
    }
    /// V(1), the portfolio value at t = 1 in state `s`.
    pub fn value1(&self, portfolio: &Portfolio, s: State) -> f64 {
        portfolio.x * self.s1(s) + portfolio.y * self.a1
    }
    /// K_V = (V(1) - V(0)) / V(0) in state `s`.
    pub fn portfolio_return(&self, portfolio: &Portfolio, s: State) -> f64 {
        (self.value1(portfolio, s) - self.value0(portfolio)) / self.value0(portfolio)
    }

    /// Expectation of a two-point random variable taking values `up`
    /// (prob. p) and `down` (prob. 1 - p).
    pub fn expect(&self, up: f64, down: f64) -> f64 {
        self.p * up + (1.0 - self.p) * down
    }
    /// Standard deviation of the same two-point random variable.
    pub fn std_dev(&self, up: f64, down: f64) -> f64 {
        (self.p * (1.0 - self.p)).sqrt() * (up - down).abs()
    }

    /// Replicating portfolio for a derivative paying `d_up` / `d_down` (D^u,
    /// D^d) at t = 1:
    ///
    /// ```text
    /// x S^u(1) + y A(1) = D^u,   x S^d(1) + y A(1) = D^d
    /// ```
    pub fn replicate(&self, d_up: f64, d_down: f64) -> Portfolio {
        let x = (d_up - d_down) / (self.s_up - self.s_down);
        let y = (d_up - x * self.s_up) / self.a1;
        Portfolio { x, y }
    }

    /// No-arbitrage price D(0) = value of the replicating portfolio ...
    pub fn price(&self, d_up: f64, d_down: f64) -> f64 {
        self.value0(&self.replicate(d_up, d_down))
    }

    /// ... which equals the discounted risk-neutral expectation.
    pub fn risk_neutral_price(&self, d_up: f64, d_down: f64) -> f64 {
        let q = self.risk_neutral_prob();
        (q * d_up + (1.0 - q) * d_down) / (1.0 + self.r())
    }

    /// Forward price for delivery at t = 1: F = S(0) (1 + r).
    pub fn forward_price(&self) -> f64 {
        self.s0 * (1.0 + self.r())
    }

    /// If the model admits arbitrage, returns a portfolio with V(0) = 0,
    /// V(1) >= 0 and V(1) > 0 in some state, using one share; otherwise
    /// `None`.
    pub fn arbitrage(&self) -> Option<Portfolio> {
        if self.r() >= self.u() {
            // Short one share and buy bonds with the proceeds.
            return Some(Portfolio {
                x: -1.0,
                y: self.s0 / self.a0,
            });
        }
        if self.r() <= self.d() {
            // Borrow to buy one share.
            return Some(Portfolio {
                x: 1.0,
                y: -self.s0 / self.a0,
            });
        }
        None
    }
}

/// Call payoff (S - X)^+ with `strike` X.
pub fn call_payoff(s: f64, strike: f64) -> f64 {
    if s > strike { s - strike } else { 0.0 }
}

/// Put payoff (X - S)^+ with `strike` X.
pub fn put_payoff(s: f64, strike: f64) -> f64 {
    if strike > s { strike - s } else { 0.0 }
}

/// Section 1.7: cost at t = 1 of buying one share for an investor who at
/// t = 0 borrowed to buy `num_calls` calls with strike `strike`:
/// S(1) - n C(1) + n C(0) (1 + r).
pub fn hedged_purchase_cost(market: &OneStepMarket, strike: f64, num_calls: f64, s: State) -> f64 {
    let call0 = market.price(
        call_payoff(market.s1(State::Up), strike),
        call_payoff(market.s1(State::Down), strike),
    );
    market.s1(s) - num_calls * call_payoff(market.s1(s), strike)
        + num_calls * call0 * (1.0 + market.r())
}
