#include "tinyclang/Lexer/TokenKind.h"

#include "llvm/ADT/StringRef.h"

namespace tinyclang::tok {

static constexpr llvm::StringLiteral TokNames[] = {
#define TOK(Name) #Name,
#include "tinyclang/Lexer/TokenKind.def"
};

auto getTokenName(enum TokenKind kind) -> llvm::StringRef {
  assert(kind < TokenKind::NUM_TOKENS);
  return TokNames[kind];
}

}  // namespace tinyclang::tok