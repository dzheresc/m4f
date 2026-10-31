#!/usr/bin/env bash
# Checks chapter 5 against the Google style guides. Tools that are not
# installed are skipped. Pass --fix to reformat in place instead of checking.
#   C++     clang-format (.clang-format), cpplint (cpp/CPPLINT.cfg)
#   Python  pyink (pyproject.toml), pylint (python/pylintrc)
#   Kotlin  ktlint (.editorconfig, Android Studio style)
#   Rust    rustfmt (rust/rustfmt.toml), clippy with warnings as errors
set -uo pipefail
cd "$(dirname "$0")"

fix=0
[[ "${1:-}" == "--fix" ]] && fix=1
status=0

run() {
  local tool="$1"
  shift
  if ! command -v "${tool}" >/dev/null; then
    echo "skip: ${tool} not installed"
    return
  fi
  if "$@"; then
    echo "ok:   ${tool}"
  else
    echo "FAIL: ${tool}"
    status=1
  fi
}

cpp_files=(cpp/include/m4f/*.h cpp/src/*.cc cpp/tests/*.cc cpp/benchmarks/*.cc)
if ((fix)); then
  run clang-format clang-format -i "${cpp_files[@]}"
  run pyink pyink --quiet --config pyproject.toml python
  run ktlint ktlint --format --relative "kotlin/src/**/*.kt"
  run cargo bash -c 'cd rust && cargo fmt'
  exit "${status}"
fi

run clang-format clang-format --dry-run --Werror "${cpp_files[@]}"
run cpplint bash -c 'cd cpp && cpplint --quiet --recursive include src tests benchmarks'
run pyink pyink --quiet --check --config pyproject.toml python
run pylint bash -c 'cd python && pylint --rcfile=pylintrc --score=n *.py'
run ktlint ktlint --relative "kotlin/src/**/*.kt"
run cargo bash -c 'cd rust && cargo fmt --check &&
  cargo clippy -q --all-targets -- -D warnings'
exit "${status}"
