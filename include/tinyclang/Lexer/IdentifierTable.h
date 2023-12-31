#ifndef TINYCLANG_LEXER_IDENTIFIERTABLE_H
#define TINYCLANG_LEXER_IDENTIFIERTABLE_H

#include <string>

#include "tinyclang/Lexer/TokenKind.h"

namespace tinyclang {

class IdentifierTable;
class MacroInfo;

/// IdentifierTokenInfo - One of these records is kept for each identifier that
/// is lexed.  This contains information about whether the token was #define'd,
/// is a language keyword, or if it is a front-end token of some sort (e.g. a
/// variable or function name).  The preprocessor keeps this information in a
/// set, and all tok::identifier tokens have a pointer to one of these.
class IdentifierTokenInfo {
  unsigned NameLen;            // String that is the identifier.
  MacroInfo* Macro;            // Set if this identifier is #define'd.
  tok::TokenKind TokenID : 8;  // Nonzero if this is a front-end token.
  bool IsExtension : 1;        // True if this token is a language extension.
  void* FETokenInfo;           // Managed by the language front-end.
  friend class IdentifierTable;

 public:
  /// getName - Return the actual string for this identifier.  The length of
  /// this string is stored in NameLen, and the returned string is properly null
  /// terminated.
  ///
  const char* getName() const {
    // String data is stored immediately after the IdentifierTokenInfo object.
    return reinterpret_cast<const char*>(this + 1);
  }

  /// getNameLength - Return the length of the identifier string.
  ///
  unsigned getNameLength() const { return NameLen; }

  /// getMacroInfo - Return macro information about this identifier, or null if
  /// it is not a macro.
  MacroInfo* getMacroInfo() const { return Macro; }
  void setMacroInfo(MacroInfo* i) { Macro = i; }

  /// get/setTokenID - If this is a source-language token (e.g. 'for'), this API
  /// can be used to cause the lexer to map identifiers to source-language
  /// tokens.
  tok::TokenKind getTokenID() const { return TokenID; }
  void setTokenID(tok::TokenKind id) { TokenID = id; }

  /// get/setExtension - Initialize information about whether or not this
  /// language token is an extension.  This controls extension warnings, and is
  /// only valid if a custom token ID is set.
  bool isExtensionToken() const { return IsExtension; }
  void setIsExtensionToken(bool val) { IsExtension = val; }

  /// getFETokenInfo/setFETokenInfo - The language front-end is allowed to
  /// associate arbitrary metadata with this token.
  template <typename T>
  T* getFETokenInfo() const {
    return static_cast<T*>(FETokenInfo);
  }
  void setFETokenInfo(void* t) { FETokenInfo = t; }

 private:
  void Destroy();
};

/// IdentifierTable - This table implements an efficient mapping from strings to
/// IdentifierTokenInfo nodes.  It has no other purpose, but this is an
/// extremely performance-critical piece of the code, as each occurrance of
/// every identifier goes through here when lexed.
class IdentifierTable {
  void* TheTable;
  void* TheMemory;
  unsigned NumIdentifiers;

 public:
  IdentifierTable();
  ~IdentifierTable();
  /// get - Return the identifier token info for the specified named identifier.
  ///
  IdentifierTokenInfo& get(const char* NameStart, const char* NameEnd);
  IdentifierTokenInfo& get(const std::string& Name);

  /// PrintStats - Print some statistics to stderr that indicate how well the
  /// hashing is doing.
  void PrintStats() const;
};

}  // namespace tinyclang

#endif  // TINYCLANG_LEXER_IDENTIFIERTABLE_H