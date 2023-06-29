#include "tinyclang/Source/SourceManager.h"

#include <algorithm>
#include <iostream>

#include "llvm/Support/Path.h"
#include "tinyclang/Basic/FileManager.h"

namespace tinyclang {

SourceManager::~SourceManager() {
  for (auto& file_info : FileInfos) {
    delete file_info.second.Buffer;
    delete[] file_info.second.SourceLineCache;
  }

  for (auto& mem_buffer_info : MemBufferInfos) {
    delete mem_buffer_info.second.Buffer;
    delete[] mem_buffer_info.second.SourceLineCache;
  }
}

/// getFileInfo - Create or return a cached FileInfo for the specified file.
///
const SourceManager::InfoRec* SourceManager::getInfoRec(
    const FileEntry* file_ent) {
  assert(file_ent && "Didn't specify a file entry to use?");
  // Do we already have information about this file?
  auto i = FileInfos.lower_bound(file_ent);
  if (i != FileInfos.end() && i->first == file_ent) {
    return &*i;
  }

  // Nope, get information.
  const llvm::MemoryBuffer* file;
  try {
    file = llvm::MemoryBuffer::getFile(file_ent->getName()).get().get();
    if (file == nullptr) {
      return nullptr;
    }
  } catch (...) {
    return nullptr;
  }

  const InfoRec& entry =
      *FileInfos.insert(i, std::make_pair(file_ent, FileInfo()));
  auto& info = const_cast<FileInfo&>(entry.second);

  info.Buffer = file;
  info.SourceLineCache = nullptr;
  info.NumLines = 0;
  return &entry;
}

/// createMemBufferInfoRec - Create a new info record for the specified memory
/// buffer.  This does no caching.
const SourceManager::InfoRec* SourceManager::createMemBufferInfoRec(
    const llvm::MemoryBuffer* buffer) {
  // Add a new info record to the MemBufferInfos list and return it.
  FileInfo fi;
  fi.Buffer = buffer;
  fi.SourceLineCache = nullptr;
  fi.NumLines = 0;
  MemBufferInfos.push_back(InfoRec(0, fi));
  return &MemBufferInfos.back();
}

/// createFileID - Create a new fileID for the specified InfoRec and include
/// position.  This works regardless of whether the InfoRec corresponds to a
/// file or some other input source.
unsigned SourceManager::createFileID(const InfoRec* file,
                                     SourceLocation include_pos) {
  // If FileEnt is really large (e.g. it's a large .i file), we may not be able
  // to fit an arbitrary position in the file in the FilePos field.  To handle
  // this, we create one FileID for each chunk of the file that fits in a
  // FilePos field.
  unsigned file_size = file->second.Buffer->getBufferSize();
  if (file_size + 1 < (1 << SourceLocation::FilePosBits)) {
    FileIDs.push_back(FileIDInfo(include_pos, 0, file));
    return FileIDs.size();
  }

  // Create one FileID for each chunk of the file.
  unsigned result = FileIDs.size() + 1;

  unsigned chunk_no = 0;
  while (true) {
    FileIDs.push_back(FileIDInfo(include_pos, chunk_no++, file));

    if (file_size + 1 < (1 << SourceLocation::FilePosBits)) {
      break;
    }
    file_size -= (1 << SourceLocation::FilePosBits);
  }

  return result;
}

/// getColumnNumber - Return the column # for the specified include position.
/// this is significantly cheaper to compute than the line number.  This returns
/// zero if the column number isn't known.
unsigned SourceManager::getColumnNumber(SourceLocation include_pos) const {
  unsigned file_id = include_pos.getFileID();
  if (file_id == 0) {
    return 0;
  }
  FileInfo* file_info = getFileInfo(file_id);
  unsigned file_pos = getFilePos(include_pos);
  const llvm::MemoryBuffer* buffer = file_info->Buffer;
  const char* buf = buffer->getBufferStart();

  unsigned line_start = file_pos;
  while (line_start && buf[line_start - 1] != '\n' &&
         buf[line_start - 1] != '\r') {
    --line_start;
  }
  return file_pos - line_start + 1;
}

/// getLineNumber - Given a SourceLocation, return the physical line number
/// for the position indicated.  This requires building and caching a table of
/// line offsets for the SourceBuffer, so this is not cheap: use only when
/// about to emit a diagnostic.
unsigned SourceManager::getLineNumber(SourceLocation include_pos) {
  FileInfo* file_info = getFileInfo(include_pos.getFileID());

  // If this is the first use of line information for this buffer, compute the
  /// SourceLineCache for it on demand.
  if (file_info->SourceLineCache == nullptr) {
    const llvm::MemoryBuffer* buffer = file_info->Buffer;

    // Find the file offsets of all of the *physical* source lines.  This does
    // not look at trigraphs, escaped newlines, or anything else tricky.
    std::vector<unsigned> line_offsets;

    // Line #1 starts at char 0.
    line_offsets.push_back(0);

    const auto* buf =
        reinterpret_cast<const unsigned char*>(buffer->getBufferStart());
    const auto* end =
        reinterpret_cast<const unsigned char*>(buffer->getBufferEnd());
    unsigned offs = 0;
    while (true) {
      // Skip over the contents of the line.
      // TODO: Vectorize this?  This is very performance sensitive for programs
      // with lots of diagnostics.
      const auto* next_buf = static_cast<const unsigned char*>(buf);
      while (*next_buf != '\n' && *next_buf != '\r' && *next_buf != '\0') {
        ++next_buf;
      }
      offs += next_buf - buf;
      buf = next_buf;

      if (buf[0] == '\n' || buf[0] == '\r') {
        // If this is \n\r or \r\n, skip both characters.
        if ((buf[1] == '\n' || buf[1] == '\r') && buf[0] != buf[1]) {
          ++offs, ++buf;
        }
        ++offs, ++buf;
        line_offsets.push_back(offs);
      } else {
        // Otherwise, this is a null.  If end of file, exit.
        if (buf == end) {
          break;
        }
        // Otherwise, skip the null.
        ++offs, ++buf;
      }
    }
    line_offsets.push_back(offs);

    // Copy the offsets into the FileInfo structure.
    file_info->NumLines = line_offsets.size();
    file_info->SourceLineCache = new unsigned[line_offsets.size()];
    std::copy(line_offsets.begin(), line_offsets.end(),
              file_info->SourceLineCache);
  }

  // Okay, we know we have a line number table.  Do a binary search to find the
  // line number that this character position lands on.
  unsigned num_lines = file_info->NumLines;
  unsigned* source_line_cache = file_info->SourceLineCache;

  // TODO: If this is performance sensitive, we could try doing simple radix
  // type approaches to make good (tight?) initial guesses based on the
  // assumption that all lines are the same average size.
  unsigned* pos =
      std::lower_bound(source_line_cache, source_line_cache + num_lines,
                       getFilePos(include_pos) + 1);
  return pos - source_line_cache;
}

/// PrintStats - Print statistics to stderr.
///
void SourceManager::PrintStats() const {
  std::cerr << "\n*** Source Manager Stats:\n";
  std::cerr << FileInfos.size() << " files mapped, " << MemBufferInfos.size()
            << " mem buffers mapped, " << FileIDs.size()
            << " file ID's allocated.\n";

  unsigned num_line_nums_computed = 0;
  unsigned num_file_bytes_mapped = 0;
  for (const auto& file_info : FileInfos) {
    num_line_nums_computed += file_info.second.SourceLineCache != nullptr;
    num_file_bytes_mapped += file_info.second.Buffer->getBufferSize();
  }
  std::cerr << num_file_bytes_mapped << " bytes of files mapped, "
            << num_line_nums_computed << " files with line #'s computed.\n";
}

}  // namespace tinyclang