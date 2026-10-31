//! Exercise 1.3: spotting a risk-free profit among currency dealers' quotes.

/// A quote for `base` in `quote` currency: the dealer buys 1 base for `buy`
/// units of quote and sells 1 base for `sell` units of quote (buy < sell).
#[derive(Clone, Debug, PartialEq)]
pub struct FxQuote {
    /// Dealer's name.
    pub dealer: String,
    /// Currency being priced.
    pub base: String,
    /// Currency the price is in.
    pub quote: String,
    /// Units of quote the dealer pays for 1 base.
    pub buy: f64,
    /// Units of quote the dealer charges for 1 base.
    pub sell: f64,
}

impl FxQuote {
    /// Builds a quote from string slices.
    pub fn new(dealer: &str, base: &str, quote: &str, buy: f64, sell: f64) -> Self {
        Self {
            dealer: dealer.into(),
            base: base.into(),
            quote: quote.into(),
            buy,
            sell,
        }
    }
}

/// Converting 1 unit of `from` into `rate` units of `to` with `dealer`.
#[derive(Clone, Debug, PartialEq)]
pub struct FxLeg {
    /// Currency paid.
    pub from: String,
    /// Currency received.
    pub to: String,
    /// Dealer's name.
    pub dealer: String,
    /// Units of `to` per unit of `from`.
    pub rate: f64,
}

/// A round trip of conversions. The legs borrow from the `FxMarket` that
/// found them, so a search copies no strings.
#[derive(Clone, Debug, PartialEq)]
pub struct FxCycle<'a> {
    /// The conversions, in order.
    pub legs: Vec<&'a FxLeg>,
    /// Units of start currency received per unit invested.
    pub growth: f64,
}

/// All dealers' quotes, indexed for the cycle search.
#[derive(Clone, Debug)]
pub struct FxMarket {
    legs: Vec<FxLeg>,
    /// Sorted, no duplicates.
    currencies: Vec<String>,
    /// n x n table, row = from, column = to, of indices into `legs`. Built
    /// once by the constructor so that the search does no string
    /// comparisons.
    best_leg: Vec<Option<u32>>,
}

impl FxMarket {
    /// Builds the market from the dealers' quotes.
    pub fn new(quotes: &[FxQuote]) -> Self {
        let mut legs = Vec::with_capacity(2 * quotes.len());
        let mut currencies = Vec::with_capacity(2 * quotes.len());
        for q in quotes {
            // Sell base to the dealer, or buy base from the dealer.
            legs.push(FxLeg {
                from: q.base.clone(),
                to: q.quote.clone(),
                dealer: q.dealer.clone(),
                rate: q.buy,
            });
            legs.push(FxLeg {
                from: q.quote.clone(),
                to: q.base.clone(),
                dealer: q.dealer.clone(),
                rate: 1.0 / q.sell,
            });
            currencies.push(q.base.clone());
            currencies.push(q.quote.clone());
        }
        currencies.sort_unstable();
        currencies.dedup();

        let mut market = Self {
            legs,
            currencies,
            best_leg: Vec::new(),
        };
        // Best leg per (from, to); legs are scanned in order and only a
        // strictly better rate replaces the current best, so the first
        // dealer wins ties.
        let n = market.currencies.len();
        let mut best_leg = vec![None; n * n];
        for (k, leg) in market.legs.iter().enumerate() {
            let i = market
                .currency_index(&leg.from)
                .expect("leg currency is listed");
            let j = market
                .currency_index(&leg.to)
                .expect("leg currency is listed");
            let best: &mut Option<u32> = &mut best_leg[i * n + j];
            if best.is_none_or(|b| leg.rate > market.legs[b as usize].rate) {
                *best = Some(k as u32);
            }
        }
        market.best_leg = best_leg;
        market
    }

    /// Best direct conversion `from` -> `to` (first dealer wins ties), or
    /// `None` if no dealer quotes that pair.
    pub fn best_leg(&self, from: &str, to: &str) -> Option<&FxLeg> {
        let i = self.currency_index(from)?;
        let j = self.currency_index(to)?;
        self.best_leg_index(i, j).map(|k| &self.legs[k])
    }

    /// Most profitable round trip start -> ... -> start through distinct
    /// currencies (exhaustive search; fine for a handful of currencies).
    pub fn best_cycle(&self, start: &str) -> FxCycle<'_> {
        let Some(s) = self.currency_index(start) else {
            return FxCycle {
                legs: Vec::new(),
                growth: 0.0,
            };
        };
        let mut search = Search {
            market: self,
            start: s,
            visited: vec![false; self.currencies.len()],
            path: Vec::new(),
            best_path: Vec::new(),
            best_growth: 0.0,
        };
        search.visited[s] = true;
        search.run(s, 1.0);
        FxCycle {
            legs: search.best_path.iter().map(|&k| &self.legs[k]).collect(),
            growth: search.best_growth,
        }
    }

    /// Index of `currency` in `currencies`, or `None` if it is not quoted.
    fn currency_index(&self, currency: &str) -> Option<usize> {
        self.currencies
            .binary_search_by(|c| c.as_str().cmp(currency))
            .ok()
    }

    /// Index into `legs` of the best leg from currency `i` to currency `j`.
    fn best_leg_index(&self, i: usize, j: usize) -> Option<usize> {
        self.best_leg[i * self.currencies.len() + j].map(|k| k as usize)
    }
}

/// Depth-first search state for `FxMarket::best_cycle`, by currency and leg
/// index.
struct Search<'a> {
    market: &'a FxMarket,
    start: usize,
    /// Per currency.
    visited: Vec<bool>,
    /// Leg indices.
    path: Vec<usize>,
    best_path: Vec<usize>,
    best_growth: f64,
}

impl Search<'_> {
    /// Extends `path` from currency `at`.
    fn run(&mut self, at: usize, growth: f64) {
        let n = self.market.currencies.len();
        for next in 0..n {
            let Some(k) = self.market.best_leg_index(at, next) else {
                continue;
            };
            let closes = next == self.start;
            if (!closes && self.visited[next]) || (closes && self.path.is_empty()) {
                continue;
            }
            self.path.push(k);
            let new_growth = growth * self.market.legs[k].rate;
            if closes {
                if new_growth > self.best_growth {
                    self.best_growth = new_growth;
                    self.best_path.clone_from(&self.path);
                }
            } else {
                self.visited[next] = true;
                self.run(next, new_growth);
                self.visited[next] = false;
            }
            self.path.pop();
        }
    }
}
