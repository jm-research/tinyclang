#include "tinyclang/Lexer/MacroExpander.h"
#include "tinyclang/Lexer/MacroInfo.h"
#include "tinyclang/Lexer/Preprocessor.h"

namespace tinyclang {

/// Lex - Lex and return a token from this macro stream.
void MacroExpander::Lex(LexerToken& Tok) {
  // Lexing off the end of the macro, pop this macro off the expansion stack.
  if (CurToken == Macro.getNumTokens())
    return PP.HandleEndOfMacro(Tok);

  // Get the next token to return.
  Tok = Macro.getReplacementToken(CurToken++);

  // If this is the first token, set the lexical properties of the token to
  // match the lexical properties of the macro identifier.
  if (CurToken == 1) {
    Tok.SetFlagValue(LexerToken::StartOfLine, AtStartOfLine);
    Tok.SetFlagValue(LexerToken::LeadingSpace, HasLeadingSpace);
  }

  // Handle recursive expansion!
  if (Tok.getIdentifierInfo())
    return PP.HandleIdentifier(Tok);

  // Otherwise, return a normal token.
}

}  // namespace tinyclang