//! Shared benchmark plumbing: a runner trait, so that the same benchmark
//! cases drive both Criterion (wall-clock time) and the callgrind runner
//! (instruction counts), and a small reproducible random number generator.
//!
//! This file is included with `#[path]` by `benches/ch03_benchmark.rs` and
//! `examples/bench_ir.rs`; each uses a different subset of it.
#![allow(dead_code)]

use std::hint::black_box;

/// Size of the input tables that cases cycle through. A power of two, so
/// that wrapping around is a mask.
pub const NUM_INPUTS: usize = 1024;

/// Returns an operation that applies `op` to the next item of `table` (which
/// must have `NUM_INPUTS` items) on each call. The table becomes a
/// fixed-size array, so the masked index needs no bounds check; the C++
/// suites index their tables the same way.
pub fn cycle<T, R>(table: Vec<T>, mut op: impl FnMut(&T) -> R) -> impl FnMut() -> R {
    let table: Box<[T; NUM_INPUTS]> = table
        .try_into()
        .unwrap_or_else(|_| panic!("a table needs {NUM_INPUTS} items"));
    let mut i = 0;
    move || {
        let r = op(&table[i]);
        i = (i + 1) & (NUM_INPUTS - 1);
        r
    }
}

/// `NUM_INPUTS` draws from `draw`.
pub fn table<T>(mut draw: impl FnMut() -> T) -> Vec<T> {
    (0..NUM_INPUTS).map(|_| draw()).collect()
}

/// Something that runs named benchmark cases.
pub trait Runner {
    /// Runs the case `name`. `make` does the setup and returns the operation
    /// to time; it is only called if the runner wants this case.
    fn bench<R, F: FnMut() -> R>(&mut self, name: &str, make: impl FnOnce() -> F);
}

/// Runs one named case a fixed number of times, for callgrind.
pub struct IrRunner {
    /// Name of the case to run.
    pub name: String,
    /// Number of iterations.
    pub iterations: u64,
    /// Whether the case was found.
    pub found: bool,
}

impl Runner for IrRunner {
    fn bench<R, F: FnMut() -> R>(&mut self, name: &str, make: impl FnOnce() -> F) {
        if name != self.name {
            return;
        }
        self.found = true;
        let mut op = make();
        for _ in 0..self.iterations {
            black_box(op());
        }
    }
}

/// Times one named case for about `seconds` and prints seconds per
/// iteration (used to pick callgrind iteration counts).
pub struct TimeRunner {
    /// Name of the case to run.
    pub name: String,
    /// Target run time.
    pub seconds: f64,
}

impl Runner for TimeRunner {
    fn bench<R, F: FnMut() -> R>(&mut self, name: &str, make: impl FnOnce() -> F) {
        if name != self.name {
            return;
        }
        let mut op = make();
        let mut iterations: u64 = 1;
        loop {
            let start = std::time::Instant::now();
            for _ in 0..iterations {
                black_box(op());
            }
            let elapsed = start.elapsed().as_secs_f64();
            if elapsed >= self.seconds || iterations >= 1 << 40 {
                println!("{:e}", elapsed / iterations as f64);
                return;
            }
            iterations *= 2;
        }
    }
}

/// Prints the name of every case.
pub struct ListRunner;

impl Runner for ListRunner {
    fn bench<R, F: FnMut() -> R>(&mut self, name: &str, _make: impl FnOnce() -> F) {
        println!("{name}");
    }
}

/// SplitMix64: a tiny reproducible generator for benchmark inputs. (The C++
/// benchmarks use std::mt19937_64; the inputs are drawn from the same ranges
/// but are not the same numbers.)
pub struct Rng(u64);

impl Rng {
    /// A generator seeded with `seed`.
    pub fn new(seed: u64) -> Self {
        Self(seed)
    }

    /// The next 64 random bits.
    pub fn next_u64(&mut self) -> u64 {
        self.0 = self.0.wrapping_add(0x9e37_79b9_7f4a_7c15);
        let mut z = self.0;
        z = (z ^ (z >> 30)).wrapping_mul(0xbf58_476d_1ce4_e5b9);
        z = (z ^ (z >> 27)).wrapping_mul(0x94d0_49bb_1331_11eb);
        z ^ (z >> 31)
    }

    /// Uniform on [lo, hi).
    pub fn uniform(&mut self, lo: f64, hi: f64) -> f64 {
        let unit = (self.next_u64() >> 11) as f64 * (1.0 / (1u64 << 53) as f64);
        lo + (hi - lo) * unit
    }

    /// Normal with the given mean and standard deviation (Box-Muller).
    pub fn normal(&mut self, mean: f64, sd: f64) -> f64 {
        let u1 = 1.0 - self.uniform(0.0, 1.0);
        let u2 = self.uniform(0.0, 1.0);
        mean + sd * (-2.0 * u1.ln()).sqrt() * (std::f64::consts::TAU * u2).cos()
    }

    /// Uniform integer on [lo, hi].
    pub fn int(&mut self, lo: i64, hi: i64) -> i64 {
        lo + (self.next_u64() % (hi - lo + 1) as u64) as i64
    }
}
