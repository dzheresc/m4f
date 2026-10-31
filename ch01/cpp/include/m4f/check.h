// Precondition checks without exceptions.
//
// The Google C++ style guide does not allow exceptions, and Google code checks
// preconditions with CHECK() from Abseil, which logs and aborts. This header
// gives the same behaviour without pulling in Abseil.
#ifndef M4F_CH01_CHECK_H_
#define M4F_CH01_CHECK_H_

#include <cstdio>
#include <cstdlib>

namespace m4f::ch01 {

// Prints `message` to stderr and aborts the program if `condition` is false.
inline void Check(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "Check failed: %s\n", message);
    std::abort();
  }
}

}  // namespace m4f::ch01

#endif  // M4F_CH01_CHECK_H_
