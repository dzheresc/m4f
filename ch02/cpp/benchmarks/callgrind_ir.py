#!/usr/bin/env python3
"""Instruction counts per benchmark iteration, measured with callgrind.

Wall-clock timings on a shared or virtual machine are noisy. Callgrind counts
executed instructions (Ir) instead, which is deterministic for a given binary,
so the numbers can be compared run to run and between code changes.

For each benchmark, the binary runs under callgrind twice with a fixed number
of iterations (N1, then N2), collecting only inside the benchmark function.
(Ir(N2) - Ir(N1)) / (N2 - N1) is the cost of one iteration with the fixed
setup cost cancelled out.

Usage: callgrind_ir.py path/to/ch02_benchmark [benchmark_filter_regex]
"""

import json
import os
import re
import subprocess
import sys
import tempfile

# Aim for this much native run time in the N2 - N1 extra iterations.
_TARGET_SECONDS = 0.005


def _list_benchmarks(binary, pattern):
    out = subprocess.run(
            [binary, "--benchmark_list_tests", f"--benchmark_filter={pattern}"],
            check=True,
            capture_output=True,
            text=True,
    ).stdout
    return out.split()


def _native_seconds(binary, name):
    """One quick native timing, used only to pick iteration counts."""
    out = subprocess.run(
            [
                    binary,
                    f"--benchmark_filter=^{re.escape(name)}$",
                    "--benchmark_min_time=0.05s",
                    "--benchmark_format=json",
            ],
            check=True,
            capture_output=True,
            text=True,
    ).stdout
    run = json.loads(out)["benchmarks"][0]
    scale = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1.0}[run["time_unit"]]
    return run["cpu_time"] * scale


def _ir(binary, name, iterations, workdir):
    function = name.split("/")[0]
    out_file = os.path.join(workdir, "callgrind.out")
    subprocess.run(
            [
                    "valgrind",
                    "--tool=callgrind",
                    f"--callgrind-out-file={out_file}",
                    "--collect-atstart=no",
                    f"--toggle-collect=*::{function}(*",
                    binary,
                    f"--benchmark_filter=^{re.escape(name)}$",
                    f"--benchmark_min_time={iterations}x",
            ],
            check=True,
            capture_output=True,
    )
    with open(out_file, encoding="utf-8") as f:
        for line in f:
            if line.startswith(("summary:", "totals:")):
                return int(line.split()[1])
    raise RuntimeError(f"no instruction total for {name}")


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    binary = os.path.abspath(argv[1])
    pattern = argv[2] if len(argv) > 2 else "."
    print(f"{'Benchmark':<28} {'Ir/iter':>14}")
    print("-" * 43)
    with tempfile.TemporaryDirectory() as workdir:
        for name in _list_benchmarks(binary, pattern):
            seconds = _native_seconds(binary, name)
            delta = max(5, min(100_000, round(_TARGET_SECONDS / seconds)))
            n1 = max(1, delta // 10)
            n2 = n1 + delta
            ir1 = _ir(binary, name, n1, workdir)
            ir2 = _ir(binary, name, n2, workdir)
            per_iter = (ir2 - ir1) / delta
            print(f"{name:<28} {per_iter:>14,.1f}", flush=True)


if __name__ == "__main__":
    main(sys.argv)
