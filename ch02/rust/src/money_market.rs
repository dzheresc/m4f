//! Section 2.2, money market: zero-coupon bonds, coupon bonds and the money
//! market account. Bond prices are per unit face value unless a face value
//! is given.
//!
//! Book notation: T is the maturity (`maturity`), B a unit bond price
//! (`bond_price`), F the face value (`face`), C the coupon (`coupon`), y the
//! yield per period, A(0) the initial balance (`a0`).

/// 2.2.1 Zero-coupon bonds. Price at 0 of a unit bond maturing at T, for an
/// implied rate r compounded m times a year (`None`: continuously).
pub fn zero_coupon_price(r: f64, maturity: f64, m: Option<i32>) -> f64 {
    match m {
        None => (-r * maturity).exp(),
        Some(m) => {
            let m = f64::from(m);
            (1.0 + r / m).powf(-maturity * m)
        }
    }
}

/// Rate implied by a unit bond price B with maturity T (inverse of
/// `zero_coupon_price`).
pub fn implied_rate(bond_price: f64, maturity: f64, m: Option<i32>) -> f64 {
    match m {
        None => -bond_price.ln() / maturity,
        Some(m) => {
            let m = f64::from(m);
            m * (bond_price.powf(-1.0 / (maturity * m)) - 1.0)
        }
    }
}

/// 2.2.2 Coupon bonds: n coupons C at the end of each period, face value F
/// paid with the last coupon; y is the yield per period.
///
/// The coupons form a geometric series, so
/// price = F v + C (1 - v) / y with v = (1+y)^-n,
/// and 1 - v is computed as -expm1(-n log1p(y)) to stay accurate for small
/// y.
pub fn coupon_bond_price(face: f64, coupon: f64, n: i32, y: f64) -> f64 {
    if y == 0.0 {
        return face + coupon * f64::from(n);
    }
    let log_v = -f64::from(n) * y.ln_1p();
    face * log_v.exp() - coupon * log_v.exp_m1() / y
}

/// Derivative of `coupon_bond_price` in y:
/// -(n F v + C sum_k k (1+y)^-k) / (1+y).
pub fn coupon_bond_price_slope(face: f64, coupon: f64, n: i32, y: f64) -> f64 {
    let discount = 1.0 / (1.0 + y);
    let mut factor = 1.0;
    let mut weighted = 0.0;
    // A half-open range: `1..=n` keeps an extra "exhausted" flag that LLVM
    // does not optimise away, which cost 40% more instructions here.
    for k in 1..n + 1 {
        factor *= discount;
        weighted += f64::from(k) * factor;
    }
    -(f64::from(n) * face * factor + coupon * weighted) * discount
}

/// Per-period yield y in [`lo`, `hi`] with
/// `coupon_bond_price(face, coupon, n, y) == price`.
///
/// Newton's method, kept inside a shrinking bisection bracket (the price is
/// strictly decreasing in y), so it falls back to bisection whenever a
/// Newton step would leave the bracket and returns the nearer end when the
/// price is out of range. Stops once the bracket or the Newton step is
/// narrower than `tol`. `n` is the number of coupon periods. The C++
/// defaults are `lo = -0.99`, `hi = 10`, `tol = 1e-12`; see
/// `yield_to_maturity`.
pub fn yield_to_maturity_in(
    price: f64,
    face: f64,
    coupon: f64,
    n: i32,
    mut lo: f64,
    mut hi: f64,
    tol: f64,
) -> f64 {
    // Start from the textbook approximation (C + (F - P) / n) / P.
    let mut y = (coupon + (face - price) / f64::from(n)) / price;
    if !(y > lo && y < hi) {
        y = (lo + hi) / 2.0;
    }
    let mut iteration = 0;
    while iteration < 200 && hi - lo >= tol {
        let excess = coupon_bond_price(face, coupon, n, y) - price;
        if excess == 0.0 {
            return y;
        }
        if excess > 0.0 {
            lo = y;
        } else {
            hi = y;
        }
        let mut next = y - excess / coupon_bond_price_slope(face, coupon, n, y);
        if !(next > lo && next < hi) {
            next = (lo + hi) / 2.0;
        }
        if (next - y).abs() < tol {
            return next;
        }
        y = next;
        iteration += 1;
    }
    (lo + hi) / 2.0
}

/// `yield_to_maturity_in` with the default bracket [-0.99, 10] and
/// tolerance 1e-12.
pub fn yield_to_maturity(price: f64, face: f64, coupon: f64, n: i32) -> f64 {
    yield_to_maturity_in(price, face, coupon, n, -0.99, 10.0, 1e-12)
}

/// 2.2.3 Money market account: A(t) = A(0) e^(r t) at a constant continuous
/// rate.
pub fn money_market_value(a0: f64, r: f64, t: f64) -> f64 {
    a0 * (r * t).exp()
}

/// Money market account built by rolling over short zero-coupon bonds: at
/// each step the whole balance buys bonds at unit price B, which pay 1 per
/// bond.
pub fn money_market_rollover(a0: f64, bond_prices: &[f64]) -> f64 {
    bond_prices
        .iter()
        .fold(a0, |balance, &bond_price| balance / bond_price)
}
