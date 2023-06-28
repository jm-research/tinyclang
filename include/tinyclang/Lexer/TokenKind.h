#ifndef TINYCLANG_LEXER_TOKENKIND_H
#define TINYCLANG_LEXER_TOKENKIND_H

#include <cstdint>

namespace llvm {
class StringRef;
}  // namespace llvm

namespace tinyclang::tok {

enum TokenKind : uint8_t {
#define TOK(Name) Name,
#include "tinyclang/Lexer/TokenKind.def"
  NUM_TOKENS
};

auto getTokenName(enum TokenKind kind) -> llvm::StringRef;

}  // namespace tinyclang::tok

#endif  // TINYCLANG_LEXER_TOKENKIND_H