#ifndef TINYCLANG_BASIC_FILEMANAGER_H
#define TINYCLANG_BASIC_FILEMANAGER_H

#include <map>
#include <string>
#include <sys/types.h>

namespace tinyclang {

class FileManager;

/// DirectoryEntry - Cached information about one directory on the disk.
class DirectoryEntry {
  std::string Name;  // Name of the directory.
  DirectoryEntry() {}
  friend class FileManager;

 public:
  auto getName() const -> const std::string& { return Name; }
};

/// FileEntry - Cached information about one file on the disk.
class FileEntry {
  std::string Name;           // Name of the directory.
  off_t Size;                 // File size in bytes.
  time_t ModTime;             // Modification time of file.
  const DirectoryEntry* Dir;  // Directory file lives in.
  unsigned UID;               // A unique (small) ID for the file.

  FileEntry() {}
  friend class FileManager;

 public:
  auto getName() const -> const std::string& { return Name; }
  auto getSize() const -> off_t { return Size; }
  auto getUID() const -> unsigned { return UID; }
  auto getModificationTime() const -> time_t { return ModTime; }

  /// getDir - Return the directory the file lives in.
  auto getDir() const -> const DirectoryEntry* { return Dir; }
};

/// FileManager - Implements support for file system lookup, file system
/// caching, and directory search management.  This also handles more advanced
/// properties, such as uniquing files based on "inode", so that a file with two
/// names (e.g. symlinked) will be treated as a single file.
class FileManager {
  /// DirEntries/FileEntries - This is a cache of directory/file entries we have
  /// looked up.
  std::map<std::string, DirectoryEntry*> DirEntries;
  std::map<std::string, FileEntry*> FileEntries;

  /// UniqueDirs/UniqueFiles - Cache from ID's to existing directories/files.
  std::map<std::pair<dev_t, ino_t>, DirectoryEntry*> UniqueDirs;
  std::map<std::pair<dev_t, ino_t>, FileEntry*> UniqueFiles;

  /// NextFileUID - Each FileEntry we create is assigned a unique ID #.
  unsigned NextFileUID;

  // Statistics.
  unsigned NumDirLookups, NumFileLookups;
  unsigned NumDirCacheMisses, NumFileCacheMisses;

 public:
  FileManager() : NextFileUID(0) {
    NumDirLookups = NumFileLookups = 0;
    NumDirCacheMisses = NumFileCacheMisses = 0;
  }

  /// getDirectory - Lookup, cache, and verify the specified directory.  This
  /// returns null if the directory doesn't exist.
  auto getDirectory(const std::string& filename) -> const DirectoryEntry*;

  /// getFile - Lookup, cache, and verify the specified file.  This returns null
  /// if the file doesn't exist.
  auto getFile(const std::string& filename) -> const FileEntry*;

  void PrintStats() const;
};

}  // namespace tinyclang

#endif  // TINYCLANG_BASIC_FILEMANAGER_H