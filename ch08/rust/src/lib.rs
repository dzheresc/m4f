//! Mathematics for Finance (Capiński & Zastawniak), Chapter 8: Option
//! Pricing.
//!
//! A Rust port of the chapter's C++ code with the same API and a
//! byte-identical report (`expected_output.txt`).

pub mod binomial_pricing;
pub mod black_scholes;
pub mod report;

/// `std::max` semantics: `b` if `a < b`, else `a`. One compare and select;
/// `f64::max` also handles NaN, which costs extra instructions in the
/// option trees' inner loops.
#[inline]
pub(crate) fn max(a: f64, b: f64) -> f64 {
    if a < b { b } else { a }
}
