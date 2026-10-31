#!/usr/bin/env bash
# Build, test and run all four implementations; verify identical output.
set -euo pipefail
cd "$(dirname "$0")"

cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build cpp/build -j >/dev/null
ctest --test-dir cpp/build --output-on-failure >/dev/null
echo "C++ tests: OK"
cpp/build/ch07 > out_cpp.txt

(cd kotlin && ./gradlew -q --console=plain test)
echo "Kotlin tests: OK"
(cd kotlin && ./gradlew -q --console=plain run) > out_kotlin.txt

(cd python && python3 -m unittest -q ch07_test 2>/dev/null)
echo "Python tests: OK"
python3 python/ch07.py > out_python.txt

(cd rust && cargo test -q --release >/dev/null 2>&1) ||
  (cd rust && cargo test -q --release)
echo "Rust tests: OK"
(cd rust && cargo run -q --release --bin ch07) > out_rust.txt

for f in out_cpp.txt out_kotlin.txt out_python.txt out_rust.txt; do
  diff -q expected_output.txt "$f"
done
echo "All four outputs match expected_output.txt"
rm -f out_cpp.txt out_kotlin.txt out_python.txt out_rust.txt
