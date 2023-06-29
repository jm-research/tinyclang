#ifndef TINYCLANG_SOURCE_SOURCEBUFFER_H
#define TINYCLANG_SOURCE_SOURCEBUFFER_H

namespace llvm::sys {
class Path;
}  // namespace llvm::sys

namespace tinyclang {

class FileEntry;

class SourceBuffer {
  const char* BufferStart;  // Start of the buffer.
  const char* BufferEnd;    // End of the buffer.

  /// MustDeleteBuffer - True if we allocated this buffer.  If so, the
  /// destructor must know the delete[] it.
  bool MustDeleteBuffer;

 protected:
  SourceBuffer() : MustDeleteBuffer(false) {}
  void init(const char* BufStart, const char* BufEnd);
  void initCopyOf(const char* BufStart, const char* BufEnd);

 public:
  virtual ~SourceBuffer();

  const char* getBufferStart() const { return BufferStart; }
  const char* getBufferEnd() const { return BufferEnd; }
  unsigned getBufferSize() const { return BufferEnd - BufferStart; }

  /// getBufferIdentifier - Return an identifier for this buffer, typically the
  /// filename it was read from.
  virtual const char* getBufferIdentifier() const { return "Unknown buffer"; }

  /// getFile - Open the specified file as a SourceBuffer, returning a new
  /// SourceBuffer if successful, otherwise returning null.
  static SourceBuffer* getFile(const FileEntry* FileEnt);

  /// getMemBuffer - Open the specified memory range as a SourceBuffer.  Note
  /// that EndPtr[0] must be a null byte and be accessible!
  static SourceBuffer* getMemBuffer(const char* StartPtr, const char* EndPtr,
                                    const char* BufferName = "");

  /// getNewMemBuffer - Allocate a new SourceBuffer of the specified size that
  /// is completely initialized to zeros.  Note that the caller should
  /// initialize the memory allocated by this method.  The memory is owned by
  /// the SourceBuffer object.
  static SourceBuffer* getNewMemBuffer(unsigned Size,
                                       const char* BufferName = "");

  /// getSTDIN - Read all of stdin into a file buffer, and return it.  This
  /// fails if stdin is empty.
  static SourceBuffer* getSTDIN();
};

}  // namespace tinyclang

#endif  // TINYCLANG_SOURCE_SOURCEBUFFER_H