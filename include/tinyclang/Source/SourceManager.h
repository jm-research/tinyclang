#ifndef TINYCLANG_SOURCE_SOURCEMANAGER_H
#define TINYCLANG_SOURCE_SOURCEMANAGER_H

#include <cassert>
#include <list>
#include <map>
#include <vector>

#include "llvm/Support/MemoryBuffer.h"
#include "tinyclang/Source/SourceLocation.h"

namespace tinyclang {

class SourceManager;
class FileEntry;
class IdentifierTokenInfo;

/// SourceManager - This file handles loading and caching of source files into
/// memory.  This object owns the SourceBuffer objects for all of the loaded
/// files and assigns unique FileID's for each unique #include chain.
class SourceManager {
  /// FileInfo - Once instance of this struct is kept for every file loaded or
  /// used.  This object owns the SourceBuffer object.
  struct FileInfo {
    /// Buffer - The actual buffer containing the characters from the input
    /// file.
    const llvm::MemoryBuffer* Buffer;

    /// SourceLineCache - A new[]'d array of offsets for each source line.  This
    /// is lazily computed.
    ///
    unsigned* SourceLineCache;

    /// NumLines - The number of lines in this FileInfo.  This is only valid if
    /// SourceLineCache is non-null.
    unsigned NumLines;
  };

  using InfoRec = std::pair<const FileEntry* const, FileInfo>;

  /// FileIDInfo - Information about a FileID, basically just the file that it
  /// represents and include stack information.
  struct FileIDInfo {
    /// IncludeLoc - The location of the #include that brought in this file.
    /// This SourceLocation object has a FileId of 0 for the main file.
    SourceLocation IncludeLoc;

    /// ChunkNo - Really large files are broken up into chunks that are each
    /// (1 << SourceLocation::FilePosBits) in size.  This specifies the chunk
    /// number of this FileID.
    unsigned ChunkNo;

    /// FileInfo - Information about the file itself.
    ///
    const InfoRec* Info;

    FileIDInfo(SourceLocation il, unsigned cn, const InfoRec* inf)
        : IncludeLoc(il), ChunkNo(cn), Info(inf) {}
  };

  /// FileInfos - Memoized information about all of the files tracked by this
  /// SourceManager.
  std::map<const FileEntry*, FileInfo> FileInfos;

  /// MemBufferInfos - Information about various memory buffers that we have
  /// read in.  This is a list, instead of a vector, because we need pointers to
  /// the FileInfo objects to be stable.
  std::list<InfoRec> MemBufferInfos;

  /// FileIDs - Information about each FileID.  FileID #0 is not valid, so all
  /// entries are off by one.
  std::vector<FileIDInfo> FileIDs;

 public:
  ~SourceManager();

  /// createFileID - Create a new FileID that represents the specified file
  /// being #included from the specified IncludePosition.  This returns 0 on
  /// error and translates NULL into standard input.
  unsigned createFileID(const FileEntry* source_file,
                        SourceLocation include_pos) {
    const InfoRec* ir = getInfoRec(source_file);
    if (ir == nullptr) {
      return 0;  // Error opening file?
    }
    return createFileID(ir, include_pos);
  }

  /// createFileIDForMemBuffer - Create a new FileID that represents the
  /// specified memory buffer.  This does no caching of the buffer and takes
  /// ownership of the SourceBuffer, so only pass a SourceBuffer to this once.
  unsigned createFileIDForMemBuffer(const llvm::MemoryBuffer* buffer) {
    const InfoRec* ir = createMemBufferInfoRec(buffer);
    return createFileID(ir, SourceLocation());
  }

  /// getMacroID - Get or create a new FileID that represents a macro with the
  /// specified identifier being expanded at the specified position.  This can
  /// never fail.
  unsigned getMacroID(const IdentifierTokenInfo* identifier,
                      SourceLocation expand_pos) {
    // FIXME: Implement ID's for macro expansions!
    return expand_pos.getFileID();
  }

  /// getBuffer - Return the buffer for the specified FileID.
  ///
  const llvm::MemoryBuffer* getBuffer(unsigned file_id) {
    return getFileInfo(file_id)->Buffer;
  }

  /// getIncludeLoc - Return the location of the #include for the specified
  /// FileID.
  SourceLocation getIncludeLoc(unsigned file_id) const {
    assert(file_id - 1 < FileIDs.size() && "Invalid FileID!");
    return FileIDs[file_id - 1].IncludeLoc;
  }

  /// getFilePos - This (efficient) method returns the offset from the start of
  /// the file that the specified SourceLocation represents.
  unsigned getFilePos(SourceLocation include_pos) const {
    assert(include_pos.getFileID() - 1 < FileIDs.size() && "Invalid FileID!");
    // If this file has been split up into chunks, factor in the chunk number
    // that the FileID references.
    unsigned chunk_no = FileIDs[include_pos.getFileID() - 1].ChunkNo;
    return include_pos.getRawFilePos() +
           (chunk_no << SourceLocation::FilePosBits);
  }

  /// getColumnNumber - Return the column # for the specified include position.
  /// this is significantly cheaper to compute than the line number.  This
  /// returns zero if the column number isn't known.
  unsigned getColumnNumber(SourceLocation include_pos) const;

  /// getLineNumber - Given a SourceLocation, return the physical line number
  /// for the position indicated.  This requires building and caching a table of
  /// line offsets for the SourceBuffer, so this is not cheap: use only when
  /// about to emit a diagnostic.
  unsigned getLineNumber(SourceLocation include_pos);

  /// getFileEntryForFileID - Return the FileEntry record for the specified
  /// FileID if one exists.
  const FileEntry* getFileEntryForFileID(unsigned file_id) const {
    assert(file_id - 1 < FileIDs.size() && "Invalid FileID!");
    return FileIDs[file_id - 1].Info->first;
  }

  /// PrintStats - Print statistics to stderr.
  ///
  void PrintStats() const;

 private:
  /// createFileID - Create a new fileID for the specified InfoRec and include
  /// position.  This works regardless of whether the InfoRec corresponds to a
  /// file or some other input source.
  unsigned createFileID(const InfoRec* file, SourceLocation include_pos);

  /// getFileInfo - Create or return a cached FileInfo for the specified file.
  /// This returns null on failure.
  const InfoRec* getInfoRec(const FileEntry* file_ent);

  /// createMemBufferInfoRec - Create a new info record for the specified memory
  /// buffer.  This does no caching.
  const InfoRec* createMemBufferInfoRec(const llvm::MemoryBuffer* buffer);

  const InfoRec* getInfoRec(unsigned file_id) const {
    assert(file_id - 1 < FileIDs.size() && "Invalid FileID!");
    return FileIDs[file_id - 1].Info;
  }

  FileInfo* getFileInfo(unsigned file_id) const {
    if (const InfoRec* ir = getInfoRec(file_id)) {
      return const_cast<FileInfo*>(&ir->second);
    }
    return nullptr;
  }
  FileInfo* getFileInfo(const FileEntry* source_file) {
    if (const InfoRec* ir = getInfoRec(source_file)) {
      return const_cast<FileInfo*>(&ir->second);
    }
    return nullptr;
  }
};

}  // namespace tinyclang

#endif  // TINYCLANG_SOURCE_SOURCEMANAGER_H
