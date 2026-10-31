//! Mathematics for Finance (Capiński & Zastawniak), Chapter 3: Risky Assets.
//!
//! A Rust port of the chapter's C++ code with the same API and a
//! byte-identical report (`expected_output.txt`).

pub mod binomial_tree;
mod libm;
pub mod other_models;
pub mod report;
pub mod returns;
