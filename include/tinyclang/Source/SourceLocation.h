#ifndef TINYCLANG_SOURCE_SOURCELOCATION_H
#define TINYCLANG_SOURCE_SOURCELOCATION_H

namespace tinyclang {

class SourceLocation {
 public:
  enum { FileIDBits = 12, FilePosBits = 32 - FileIDBits };

  SourceLocation() : ID(0) {}  // 0 is an invalid FileID.

  /// SourceLocation constructor - Create a new SourceLocation object with the
  /// specified FileID and FilePos.
  SourceLocation(unsigned file_id, unsigned file_pos) {
    // If a FilePos is larger than (1<<FilePosBits), the SourceManager makes
    // enough consequtive FileIDs that we have one for each chunk.
    if (file_pos >= (1 << FilePosBits)) {
      file_id += file_pos >> FilePosBits;
      file_pos &= (1 << FilePosBits) - 1;
    }

    // FIXME: Find a way to handle out of FileID bits!  Maybe MaxFileID is an
    // escape of some sort?
    if (file_id >= (1 << FileIDBits)) {
      file_id = (1 << FileIDBits) - 1;
    }

    ID = (file_id << FilePosBits) | file_pos;
  }

  /// isValid - Return true if this is a valid SourceLocation object.  Invalid
  /// SourceLocations are often used when events have no corresponding location
  /// in the source (e.g. a diagnostic is required for a command line option).
  ///
  bool isValid() const { return ID != 0; }

  /// getFileID - Return the file identifier for this SourceLocation.  This
  /// FileID can be used with the SourceManager object to obtain an entire
  /// include stack for a file position reference.
  unsigned getFileID() const { return ID >> FilePosBits; }

  /// getRawFilePos - Return the byte offset from the start of the file-chunk
  /// referred to by FileID.  This method should not be used to get the offset
  /// from the start of the file, instead you should use
  /// SourceManager::getFilePos.  This method will be incorrect for large files.
  unsigned getRawFilePos() const { return ID & ((1 << FilePosBits) - 1); }

  /// getRawEncoding - When a SourceLocation itself cannot be used, this returns
  /// an (opaque) 32-bit integer encoding for it.  This should only be passed
  /// to SourceLocation::getFromRawEncoding, it should not be inspected
  /// directly.
  unsigned getRawEncoding() const { return ID; }

  /// getFromRawEncoding - Turn a raw encoding of a SourceLocation object into
  /// a real SourceLocation.
  static SourceLocation getFromRawEncoding(unsigned encoding) {
    SourceLocation x;
    x.ID = encoding;
    return x;
  }

 private:
  unsigned ID;
};

inline bool operator==(const SourceLocation& lhs, const SourceLocation& rhs) {
  return lhs.getRawEncoding() == rhs.getRawEncoding();
}

inline bool operator!=(const SourceLocation& lhs, const SourceLocation& rhs) {
  return !(lhs == rhs);
}

}  // namespace tinyclang

#endif  // TINYCLANG_SOURCE_SOURCELOCATION_H