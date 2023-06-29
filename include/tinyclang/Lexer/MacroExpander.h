#ifndef TINYCLANG_LEXER_MACROEXPANDER_H
#define TINYCLANG_LEXER_MACROEXPANDER_H

namespace tinyclang {

class MacroInfo;
class Preprocessor;
class LexerToken;

/// MacroExpander - This implements a lexer that returns token from a macro body
/// instead of lexing from a character buffer.
///
class MacroExpander {
  /// Macro - The macro we are expanding from.
  ///
  MacroInfo& Macro;

  /// CurMacroID - This encodes the instantiation point of the macro being
  /// expanded and the include stack.
  unsigned CurMacroID;

  /// PP - The current preprocessor object we are expanding for.
  ///
  Preprocessor& PP;

  /// CurToken - This is the next token that Lex will return.
  unsigned CurToken;

  /// Lexical information about the expansion point of the macro: the identifier
  /// that the macro expanded from had these properties.
  bool AtStartOfLine, HasLeadingSpace;

 public:
  MacroExpander(MacroInfo& macro, unsigned MacroID, Preprocessor& pp,
                bool atStartOfLine, bool hasLeadingSpace)
      : Macro(macro),
        CurMacroID(MacroID),
        PP(pp),
        CurToken(0),
        AtStartOfLine(atStartOfLine),
        HasLeadingSpace(hasLeadingSpace) {}

  MacroInfo& getMacro() const { return Macro; }

  /// Lex - Lex and return a token from this macro stream.
  void Lex(LexerToken& Tok);
};

}  // namespace tinyclang

#endif  // TINYCLANG_LEXER_MACROEXPANDER_H