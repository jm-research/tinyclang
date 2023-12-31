#include "tinyclang/Diagnostic/Diagnostic.h"

#include <cassert>

#include "tinyclang/Source/SourceLocation.h"

namespace tinyclang {

/// Flag values for diagnostics.
enum {
  // Diagnostic classes.
  NOTE = 0x01,
  WARNING = 0x02,
  EXTENSION = 0x03,
  ERROR = 0x04,
  FATAL = 0x05,
  class_mask = 0x07
};

/// DiagnosticFlags - A set of flags, or'd together, that describe the
/// diagnostic.
static unsigned char DiagnosticFlags[] = {
#define DIAG(ENUM, FLAGS, DESC) FLAGS,
#include "tinyclang/Diagnostic/DiagnosticKind.def"
};

/// getDiagClass - Return the class field of the diagnostic.
///
static unsigned getDiagClass(unsigned DiagID) {
  assert(DiagID < diag::NUM_DIAGNOSTICS && "Diagnostic ID out of range!");
  return DiagnosticFlags[DiagID] & class_mask;
}

/// DiagnosticText - An english message to print for the diagnostic.  These
/// should be localized.
static const char* const DiagnosticText[] = {
#define DIAG(ENUM, FLAGS, DESC) DESC,
#include "tinyclang/Diagnostic/DiagnosticKind.def"
};

/// isNoteWarningOrExtension - Return true if the unmapped diagnostic level of
/// the specified diagnostic ID is a Note, Warning, or Extension.
bool Diagnostic::isNoteWarningOrExtension(unsigned DiagID) {
  return getDiagClass(DiagID) < ERROR;
}

/// getDescription - Given a diagnostic ID, return a description of the
/// issue.
const char* Diagnostic::getDescription(unsigned DiagID) {
  assert(DiagID < diag::NUM_DIAGNOSTICS && "Diagnostic ID out of range!");
  return DiagnosticText[DiagID];
}

/// getDiagnosticLevel - Based on the way the client configured the Diagnostic
/// object, classify the specified diagnostic ID into a Level, consumable by
/// the DiagnosticClient.
Diagnostic::Level Diagnostic::getDiagnosticLevel(unsigned DiagID) const {
  unsigned DiagClass = getDiagClass(DiagID);

  // TODO: specific diagnostics may be enabled or disabled.  Filter those based
  // on their DiagID.

  // Map diagnostic classes based on command line argument settings.
  if (DiagClass == EXTENSION) {
    if (ErrorOnExtensions)
      DiagClass = ERROR;
    else if (WarnOnExtensions)
      DiagClass = WARNING;
    else
      return Ignored;
  }

  // If warnings are to be treated as errors, indicate this as such.
  if (DiagClass == WARNING && WarningsAsErrors)
    DiagClass = ERROR;

  switch (DiagClass) {
    default:
      assert(0 && "Unknown diagnostic class!");
    case NOTE:
      return Diagnostic::Note;
    case WARNING:
      return Diagnostic::Warning;
    case ERROR:
      return Diagnostic::Error;
    case FATAL:
      return Diagnostic::Fatal;
  }
}

/// Report - Issue the message to the client. If the client wants us to stop
/// compilation, return true, otherwise return false.  DiagID is a member of
/// the diag::kind enum.
void Diagnostic::Report(SourceLocation Pos, unsigned DiagID,
                        const std::string& Extra) {
  // Figure out the diagnostic level of this message.
  Diagnostic::Level DiagLevel = getDiagnosticLevel(DiagID);

  // If the client doesn't care about this message, don't issue it.
  if (DiagLevel == Diagnostic::Ignored)
    return;

  // Finally, report it.
  Client.HandleDiagnostic(DiagLevel, Pos, (diag::kind)DiagID, Extra);
}

DiagnosticClient::~DiagnosticClient() {}

}  // namespace tinyclang