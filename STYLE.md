# Code style

The code follows Google's style guides. Chapter 1 has been converted; the other
chapters still use the older house style and can be converted the same way.

| Language | Guide | Formatter / linter config (in `ch01/`) |
|---|---|---|
| C++ | [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) | `.clang-format` (`BasedOnStyle: Google`), `cpp/CPPLINT.cfg` |
| Python | [Google Python Style Guide](https://google.github.io/styleguide/pyguide.html) | `pyproject.toml` (pyink), `python/pylintrc` (Google's own) |
| Kotlin | [Android Kotlin style guide](https://developer.android.com/kotlin/style-guide) (Google has no general Kotlin guide) | `.editorconfig` (`ktlint_code_style = android_studio`) |

`ch01/check_style.sh` runs every tool that is installed; `--fix` reformats.

## What changed in practice

### C++

- Files are `.h` / `.cc`, tests are `*_test.cc`.
- `#define` include guards (`M4F_CH01_ONE_STEP_H_`), not `#pragma once`.
- Includes in order: own header, C system, C++ standard library, project.
- 2-space indent, 80 columns, `public:` / `private:` indented by one space.
- Names:
  - types `PascalCase`;
  - functions `PascalCase` (`Price`, `IsArbitrageFree`);
  - cheap accessors `snake_case` (`a0()`, `r()`, `s1(state)`);
  - variables and parameters `snake_case` (`s_up`, `num_calls`);
  - private members get a trailing `_`;
  - enumerators `kUp`, `kDown`.
- No exceptions. Constructor preconditions go through `Check()` in
  `m4f/check.h`, which prints and aborts like Abseil's `CHECK`.
- No `using namespace`; file-local helpers go in an unnamed namespace, not
  `static`.
- Function bodies longer than a few lines live in `.cc` files (for example
  `FxMarket` is now in `src/fx_arbitrage.cc`), and long functions are split up
  (the report is one function per book section).
- Braces on every multi-line `if` / `for`.

### Python

- `import module`, never `from module import name`.
- 4-space indent, 80 columns, formatted by pyink (Google's fork of Black).
- `snake_case` for functions, variables and parameters; no single-letter names
  except loop counters and book symbols.
- Docstrings in Google format (summary line, then `Args:` / `Returns:` /
  `Raises:`).
- No `assert` for runtime checks; raise an exception instead.
- `main()` is called from `if __name__ == "__main__":`.
- Tests are named `*_test.py`.

### Kotlin

- 4-space indent, 100 columns, trailing commas in multi-line argument lists.
- `lowerCamelCase` for functions, properties and parameters (`sUp`,
  `riskNeutralProb`).
- Enum constants are `UPPER_SNAKE_CASE` (`State.UP`).
- One statement per line.
- Braces on every `if` / `for` whose body does not fit on one line.
- KDoc (`/** ... */`) for public declarations.

## Book notation

The book writes A(0), S^u(1), D^d and so on. Upper-case names break every
guide, so the code uses the same letters in lower case:

| Book | C++ / Python | Kotlin |
|---|---|---|
| A(0), A(1), S(0) | `a0`, `a1`, `s0` | `a0`, `a1`, `s0` |
| S^u(1), S^d(1) | `s_up`, `s_down` | `sUp`, `sDown` |
| D^u, D^d | `d_up`, `d_down` | `dUp`, `dDown` |
| F (forward price) | `delivery_price` | `deliveryPrice` |
| X (strike) | `strike` | `strike` |

The book's own symbols p, r, u, d, x, y and z stay single letters. The Python
guide allows this in section 3.16.5, "Mathematical Notation".
