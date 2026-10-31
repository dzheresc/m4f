//! Runs one benchmark case a fixed number of times, so that callgrind can
//! count its instructions (see `benches/callgrind_ir.py`).
//!
//! Usage:
//!   bench_ir --list
//!   bench_ir --time NAME        seconds per iteration, measured natively
//!   bench_ir NAME ITERATIONS

#[path = "../benches/cases.rs"]
mod cases;
#[path = "../benches/harness.rs"]
mod harness;

use harness::{IrRunner, ListRunner, TimeRunner};

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match args.as_slice() {
        [flag] if flag == "--list" => cases::all(&mut ListRunner),
        [flag, name] if flag == "--time" => {
            cases::all(&mut TimeRunner {
                name: name.clone(),
                seconds: 0.05,
            });
        }
        [name, iterations] => {
            let mut runner = IrRunner {
                name: name.clone(),
                iterations: iterations.parse().expect("ITERATIONS must be an integer"),
                found: false,
            };
            cases::all(&mut runner);
            assert!(runner.found, "no benchmark case named {name}");
        }
        _ => {
            eprintln!("usage: bench_ir --list | --time NAME | NAME ITERATIONS");
            std::process::exit(2);
        }
    }
}
