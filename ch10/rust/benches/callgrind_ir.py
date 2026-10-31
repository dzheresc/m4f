#!/usr/bin/env python3
"""Instruction counts per benchmark iteration for the Rust code, by callgrind.

The Rust counterpart of cpp/benchmarks/callgrind_ir.py, so that the two
languages can be compared on the same exact metric. Callgrind counts executed
instructions (Ir), which is deterministic for a given binary.

For each case, examples/bench_ir runs under callgrind twice with a fixed
number of iterations (N1, then N2). (Ir(N2) - Ir(N1)) / (N2 - N1) is the cost
of one iteration: process start-up and the case's setup cancel out.

Usage: callgrind_ir.py path/to/bench_ir [name_regex]
"""

import os
import re
import subprocess
import sys
import tempfile

# Aim for this much native run time in the N2 - N1 extra iterations.
_TARGET_SECONDS = 0.005


def _run(args):
    return subprocess.run(
            args, check=True, capture_output=True, text=True
    ).stdout


def _ir(binary, name, iterations, workdir):
    out_file = os.path.join(workdir, "callgrind.out")
    subprocess.run(
            [
                    "valgrind",
                    "--tool=callgrind",
                    f"--callgrind-out-file={out_file}",
                    binary,
                    name,
                    str(iterations),
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
    pattern = re.compile(argv[2] if len(argv) > 2 else ".")
    names = [n for n in _run([binary, "--list"]).split() if pattern.search(n)]
    print(f"{'Benchmark':<28} {'Ir/iter':>14}")
    print("-" * 43)
    with tempfile.TemporaryDirectory() as workdir:
        for name in names:
            seconds = float(_run([binary, "--time", name]))
            delta = max(5, min(100_000, round(_TARGET_SECONDS / seconds)))
            n1 = max(1, delta // 10)
            n2 = n1 + delta
            ir1 = _ir(binary, name, n1, workdir)
            ir2 = _ir(binary, name, n2, workdir)
            print(f"{name:<28} {(ir2 - ir1) / delta:>14,.1f}", flush=True)


if __name__ == "__main__":
    main(sys.argv)
