#include "tinyclang/Basic/FileManager.h"

auto main() -> int {
  tinyclang::FileManager fm;

  fm.getFile(
      "/root/jm-research/tinyclang/unittests/Basic/FileManager.t.cc");
  fm.PrintStats();
  return 0;
}