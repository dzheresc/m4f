//! Criterion microbenchmarks for chapter 3; the cases are in `cases.rs`.

mod cases;
mod harness;

use criterion::{Criterion, criterion_group, criterion_main};
use harness::Runner;

/// Adapts Criterion to the `Runner` trait.
struct CriterionRunner<'a>(&'a mut Criterion);

impl Runner for CriterionRunner<'_> {
    fn bench<R, F: FnMut() -> R>(&mut self, name: &str, make: impl FnOnce() -> F) {
        let mut op = make();
        self.0.bench_function(name, |b| b.iter(&mut op));
    }
}

fn run_all(c: &mut Criterion) {
    cases::all(&mut CriterionRunner(c));
}

criterion_group! {
    name = benches;
    config = Criterion::default()
        .warm_up_time(std::time::Duration::from_millis(500))
        .measurement_time(std::time::Duration::from_secs(2));
    targets = run_all
}
criterion_main!(benches);
