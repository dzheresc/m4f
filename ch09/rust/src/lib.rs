//! Mathematics for Finance (Capiński & Zastawniak), Chapter 9: Financial
//! Engineering.
//!
//! A Rust port of the chapter's C++ code with the same API and a
//! byte-identical report (`expected_output.txt`).

pub mod black_scholes;
pub mod hedging;
pub mod report;
pub mod risk;

/// `std::max` semantics: `b` if `a < b`, else `a`. One compare and select;
/// `f64::max` also handles NaN.
#[inline]
pub(crate) fn max(a: f64, b: f64) -> f64 {
    if a < b { b } else { a }
}
