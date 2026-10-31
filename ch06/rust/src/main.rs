//! Prints the chapter 6 report to standard output.

use std::io::Write;

fn main() -> std::io::Result<()> {
    let mut report = String::new();
    m4f_ch06::report::print_report(&mut report).expect("writing to a String cannot fail");
    std::io::stdout().lock().write_all(report.as_bytes())
}
