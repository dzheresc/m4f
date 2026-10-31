//! Section 2.1, time value of money.
//!
//! Book notation: P is the principal (`principal`), V the future value
//! (`value`), r the annual interest rate, t the time in years, m the number
//! of compounding periods per year.
//!
//! Powers use `powf` with the exponent converted to `f64`, which is exactly
//! what C++ `std::pow(double, int)` does; `powi` would round differently.

/// 2.1.1 Simple interest: V(t) = (1 + t r) P.
pub fn simple_value(principal: f64, r: f64, t: f64) -> f64 {
    (1.0 + t * r) * principal
}

/// Present value P = V / (1 + t r) under simple interest.
pub fn simple_present_value(value: f64, r: f64, t: f64) -> f64 {
    value / (1.0 + t * r)
}

/// Rate r such that `principal` grows to `value` in time t under simple
/// interest.
pub fn simple_rate(principal: f64, value: f64, t: f64) -> f64 {
    (value / principal - 1.0) / t
}

/// 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P.
pub fn periodic_value(principal: f64, r: f64, m: i32, t: f64) -> f64 {
    let m = f64::from(m);
    (1.0 + r / m).powf(t * m) * principal
}

/// Present value P = V (1 + r/m)^(-t m) under periodic compounding.
pub fn periodic_present_value(value: f64, r: f64, m: i32, t: f64) -> f64 {
    let m = f64::from(m);
    value * (1.0 + r / m).powf(-t * m)
}

/// Effective annual rate r_e = (1 + r/m)^m - 1.
pub fn effective_rate(r: f64, m: i32) -> f64 {
    let m = f64::from(m);
    (1.0 + r / m).powf(m) - 1.0
}

// 2.1.3 Streams of payments: n payments C (`payment`) at the end of each
// period, per-period rate i (for annual payments with annual compounding,
// i = r).

/// PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments.
pub fn annuity_factor(i: f64, n: i32) -> f64 {
    (1.0 - (1.0 + i).powf(f64::from(-n))) / i
}

/// Present value of n payments C: C PA(i, n).
pub fn annuity_present_value(payment: f64, i: f64, n: i32) -> f64 {
    payment * annuity_factor(i, n)
}

/// Value at the last payment of n payments C: C ((1+i)^n - 1) / i.
pub fn annuity_future_value(payment: f64, i: f64, n: i32) -> f64 {
    payment * ((1.0 + i).powf(f64::from(n)) - 1.0) / i
}

/// Present value C / i of a perpetuity.
pub fn perpetuity_present_value(payment: f64, i: f64) -> f64 {
    payment / i
}

/// Payment C repaying a loan PA in n installments: C = PA / PA(i, n).
pub fn loan_installment(loan: f64, i: f64, n: i32) -> f64 {
    loan / annuity_factor(i, n)
}

/// 2.1.4 Continuous compounding: V(t) = e^(t r) P.
pub fn continuous_value(principal: f64, r: f64, t: f64) -> f64 {
    (t * r).exp() * principal
}

/// Present value P = V e^(-t r) under continuous compounding.
pub fn continuous_present_value(value: f64, r: f64, t: f64) -> f64 {
    value * (-t * r).exp()
}

/// Effective annual rate e^r - 1 of a continuously compounded rate.
pub fn continuous_effective_rate(r: f64) -> f64 {
    r.exp() - 1.0
}

// 2.1.5 Comparing compounding methods: rates giving the same growth. r_m is
// a rate compounded m times a year, r_c a continuously compounded rate.

/// Continuous rate r_c = m ln(1 + r_m/m) equivalent to `r_m`.
pub fn continuous_from_periodic(r_m: f64, m: i32) -> f64 {
    let m = f64::from(m);
    m * (1.0 + r_m / m).ln()
}

/// Periodic rate r_m = m (e^(r_c/m) - 1) equivalent to `r_c`.
pub fn periodic_from_continuous(r_c: f64, m: i32) -> f64 {
    let m = f64::from(m);
    m * ((r_c / m).exp() - 1.0)
}

/// Rate compounded k times a year equivalent to `r_m` compounded m times a
/// year.
pub fn periodic_from_periodic(r_m: f64, m: i32, k: i32) -> f64 {
    let (m, k) = (f64::from(m), f64::from(k));
    k * ((1.0 + r_m / m).powf(m / k) - 1.0)
}
