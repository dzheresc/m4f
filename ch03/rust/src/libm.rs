//! C math functions that Rust's standard library does not expose on stable.
//!
//! The crate denies `unsafe_code`; this module is the one exception. The
//! declaration is marked `safe` because `lgamma` is a pure function defined
//! for every `f64`, so callers need no `unsafe` block.
#![allow(unsafe_code)]

unsafe extern "C" {
    /// ln |Γ(x)| from the platform C library, the function C++
    /// `std::lgamma` calls, so both languages get the same bits.
    pub safe fn lgamma(x: f64) -> f64;
}
