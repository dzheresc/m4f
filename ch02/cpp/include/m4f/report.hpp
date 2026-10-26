#pragma once
#include <iosfwd>

namespace m4f::ch02 {
// Prints the chapter's worked examples. The text is byte-identical across the
// C++, Kotlin and Python implementations.
void printReport(std::ostream& os);
}
