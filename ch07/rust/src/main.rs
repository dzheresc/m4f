//! Prints the chapter 7 report to standard output.

use std::io::Write;

fn main() -> std::io::Result<()> {
    let mut report = String::new();
    m4f_ch07::report::print_report(&mut report).expect("writing to a String cannot fail");
    std::io::stdout().lock().write_all(report.as_bytes())
}
