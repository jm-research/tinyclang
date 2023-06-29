#include "tinyclang/Lexer/MacroInfo.h"

#include <iostream>

namespace tinyclang {

/// dump - Print the macro to stderr, used for debugging.
///
void MacroInfo::dump(const LangOptions& Features) const {
  std::cerr << "MACRO: ";
  for (unsigned i = 0, e = ReplacementTokens.size(); i != e; ++i) {
    ReplacementTokens[i].dump(Features);
    std::cerr << "  ";
  }
  std::cerr << "\n";
}

}  // namespace tinyclang