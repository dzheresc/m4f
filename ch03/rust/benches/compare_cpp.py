#!/usr/bin/env python3
"""Rust vs C++ benchmark table, in Markdown.

Wall-clock: the median of five Google Benchmark repetitions for C++, and the
Criterion median (from target/criterion, so run `cargo bench` first) for
Rust. Instructions: both callgrind_ir.py scripts. Cases are matched by name;
the C++ names carry a `BM_` prefix.

Usage: compare_cpp.py CPP_BENCHMARK RUST_BENCH_IR CRITERION_DIR [name_regex]
"""

import json
import os
import re
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
_CPP_CALLGRIND = os.path.join(
    _HERE, "..", "..", "cpp", "benchmarks", "callgrind_ir.py"
)
_RUST_CALLGRIND = os.path.join(_HERE, "callgrind_ir.py")
_SCALE_NS = {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}


def _run(args):
    return subprocess.run(
        args, check=True, capture_output=True, text=True
    ).stdout


def _cpp_times(binary, pattern):
    out = _run([
        binary,
        f"--benchmark_filter={pattern}",
        "--benchmark_repetitions=5",
        "--benchmark_report_aggregates_only=true",
        "--benchmark_format=json",
    ])
    times = {}
    for run in json.loads(out)["benchmarks"]:
        if run.get("aggregate_name") == "median":
            name = run["run_name"].removeprefix("BM_")
            times[name] = run["real_time"] * _SCALE_NS[run["time_unit"]]
    return times


def _rust_time(criterion_dir, name):
    path = os.path.join(
        criterion_dir, name.replace("/", "_"), "new", "estimates.json"
    )
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        return json.load(f)["median"]["point_estimate"]


def _ir(script, binary, pattern):
    counts = {}
    for line in _run([sys.executable, script, binary, pattern]).splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[1][0].isdigit():
            counts[parts[0].removeprefix("BM_")] = float(
                parts[1].replace(",", "")
            )
    return counts


def _fmt_time(ns):
    if ns is None:
        return "—"
    for unit, scale in (("s", 1e9), ("ms", 1e6), ("µs", 1e3)):
        if ns >= scale:
            return f"{ns / scale:.3g} {unit}"
    return f"{ns:.3g} ns"


def _ratio(rust, cpp):
    return "—" if not rust or not cpp else f"{rust / cpp:.2f}"


def main(argv):
    if len(argv) < 4:
        sys.exit(__doc__)
    cpp_binary, rust_binary, criterion_dir = argv[1:4]
    pattern = argv[4] if len(argv) > 4 else "."
    cpp_times = _cpp_times(cpp_binary, pattern)
    cpp_ir = _ir(_CPP_CALLGRIND, cpp_binary, pattern)
    rust_ir = _ir(_RUST_CALLGRIND, rust_binary, pattern)
    names = _run([rust_binary, "--list"]).split()
    print("| Benchmark | C++ time | Rust time | Rust/C++ | C++ Ir | Rust Ir |"
          " Rust/C++ |")
    print("|---|---:|---:|---:|---:|---:|---:|")
    for name in names:
        if not re.search(pattern, name):
            continue
        cpp_t = cpp_times.get(name)
        rust_t = _rust_time(criterion_dir, name)
        c_ir = cpp_ir.get(name)
        r_ir = rust_ir.get(name)
        print(
            f"| `{name}` | {_fmt_time(cpp_t)} | {_fmt_time(rust_t)} |"
            f" {_ratio(rust_t, cpp_t)} |"
            f" {'—' if c_ir is None else f'{c_ir:,.0f}'} |"
            f" {'—' if r_ir is None else f'{r_ir:,.0f}'} |"
            f" {_ratio(r_ir, c_ir)} |",
            flush=True,
        )


if __name__ == "__main__":
    main(sys.argv)
