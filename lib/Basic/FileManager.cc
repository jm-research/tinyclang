#include "tinyclang/Basic/FileManager.h"

#include <sys/stat.h>

#include <iostream>

namespace tinyclang {

/// getDirectory - Lookup, cache, and verify the specified directory.  This
/// returns null if the directory doesn't exist.
auto FileManager::getDirectory(const std::string& filename)
    -> const DirectoryEntry* {
  ++NumDirLookups;
  // See if there is already an entry in the map.
  auto i = DirEntries.lower_bound(filename);
  if (i != DirEntries.end() && i->first == filename) {
    return i->second;
  }

  ++NumDirCacheMisses;

  // By default, zero initialize it.
  DirectoryEntry*& ent =
      DirEntries.insert(i, std::make_pair(filename, (DirectoryEntry*)nullptr))
          ->second;

  // Nope, there isn't.  Check to see if the directory exists.
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) ||  // Error stat'ing.
      !S_ISDIR(stat_buf.st_mode)) {         // Not a directory?
    return nullptr;
  }

  // It exists.  See if we have already opened a directory with the same inode.
  // This occurs when one dir is symlinked to another, for example.
  DirectoryEntry*& ude =
      UniqueDirs[std::make_pair(stat_buf.st_dev, stat_buf.st_ino)];

  // Already have an entry with this inode, return it.
  if (ude) {
    return ent = ude;
  }

  // Otherwise, we don't have this directory yet, add it.
  auto* de = new DirectoryEntry();
  de->Name = filename;
  return ent = ude = de;
}

/// getFile - Lookup, cache, and verify the specified file.  This returns null
/// if the file doesn't exist.
auto FileManager::getFile(const std::string& filename) -> const FileEntry* {
  ++NumFileLookups;

  // See if there is already an entry in the map.
  auto i = FileEntries.lower_bound(filename);
  if (i != FileEntries.end() && i->first == filename) {
    return i->second;
  }

  ++NumFileCacheMisses;

  // By default, zero initialize it.
  FileEntry*& ent =
      FileEntries.insert(i, std::make_pair(filename, (FileEntry*)nullptr))
          ->second;

  // Figure out what directory it is in.
  std::string dir_name;

  // If the string contains a / in it, strip off everything after it.
  // FIXME: this logic should be in sys::Path.
  std::string::size_type slash_pos = filename.find_last_of('/');
  if (slash_pos == std::string::npos) {
    dir_name = ".";  // Use the current directory if file has no path component.
  } else if (slash_pos == filename.size() - 1) {
    return nullptr;  // If filename ends with a /, it's a directory.
  } else {
    dir_name = std::string(filename.begin(), filename.begin() + slash_pos);
  }

  const DirectoryEntry* dir_info = getDirectory(dir_name);
  if (dir_info == nullptr) {  // Directory doesn't exist, file can't exist.
    return nullptr;
  }

  // FIXME: Use the directory info to prune this, before doing the stat syscall.
  // FIXME: This will reduce the # syscalls.

  // Nope, there isn't.  Check to see if the file exists.
  struct stat stat_buf;
  // std::cerr << "STATING: " << Filename;
  if (stat(filename.c_str(), &stat_buf) ||  // Error stat'ing.
      S_ISDIR(stat_buf.st_mode)) {          // A directory?
    // If this file doesn't exist, we leave a null in FileEntries for this path.
    // std::cerr << ": Not existing\n";
    return nullptr;
  }
  // std::cerr << ": exists\n";

  // It exists.  See if we have already opened a directory with the same inode.
  // This occurs when one dir is symlinked to another, for example.
  FileEntry*& ufe =
      UniqueFiles[std::make_pair(stat_buf.st_dev, stat_buf.st_ino)];

  if (ufe) {  // Already have an entry with this inode, return it.
    return ent = ufe;
  }

  // Otherwise, we don't have this directory yet, add it.
  auto* fe = new FileEntry();
  fe->Name = filename;
  fe->Size = stat_buf.st_size;
  fe->ModTime = stat_buf.st_mtime;
  fe->Dir = dir_info;
  fe->UID = NextFileUID++;
  return ent = ufe = fe;
}

void FileManager::PrintStats() const {
  std::cerr << "\n*** File Manager Stats:\n";
  std::cerr << UniqueFiles.size() << " files found, " << UniqueDirs.size()
            << " dirs found.\n";
  std::cerr << NumDirLookups << " dir lookups, " << NumDirCacheMisses
            << " dir cache misses.\n";
  std::cerr << NumFileLookups << " file lookups, " << NumFileCacheMisses
            << " file cache misses.\n";

  // std::cerr << PagesMapped << BytesOfPagesMapped << FSLookups;
}

}  // namespace tinyclang