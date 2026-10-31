#ifndef M4F_CH07_REPORT_H_
#define M4F_CH07_REPORT_H_

#include <iosfwd>

namespace m4f::ch07 {

// Prints the chapter's worked examples. The text is byte-identical across the
// C++, Kotlin and Python implementations.
void PrintReport(std::ostream& os);

}  // namespace m4f::ch07

#endif  // M4F_CH07_REPORT_H_
