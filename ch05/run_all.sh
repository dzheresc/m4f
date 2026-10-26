#!/usr/bin/env bash
# Build, test and run all three implementations; verify identical output.
set -euo pipefail
cd "$(dirname "$0")"

cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build cpp/build -j >/dev/null
ctest --test-dir cpp/build --output-on-failure >/dev/null
echo "C++ tests: OK"
cpp/build/ch05 > out_cpp.txt

(cd kotlin && ./gradlew -q --console=plain test)
echo "Kotlin tests: OK"
(cd kotlin && ./gradlew -q --console=plain run) > out_kotlin.txt

(cd python && python3 -m unittest -q test_ch05 2>/dev/null)
echo "Python tests: OK"
python3 python/ch05.py > out_python.txt

for f in out_cpp.txt out_kotlin.txt out_python.txt; do diff -q expected_output.txt "$f"; done
echo "All three outputs match expected_output.txt"
rm -f out_cpp.txt out_kotlin.txt out_python.txt
