#include <cstdlib>
#include <cstring>

#include "sdllib/memflag.h"

void (*Memory_Error)() = nullptr;
void (*Memory_Error_Exit)(char* string) = nullptr;

void Force_VM_Page_In(void* /*buffer*/, int /*length*/) {}

char* Alloc(const unsigned long bytes_to_alloc, const MemoryFlagType flags) {
  return (flags & MEM_CLEAR) ? new char[bytes_to_alloc]()
                             : new char[bytes_to_alloc];
}

void Free(const void* pointer) {
  if (pointer) {
    delete[] (char*)pointer;
  }
}

void Mem_Copy(const void* source, void* dest, unsigned long bytes_to_copy) {
  memcpy(dest, source, bytes_to_copy);
}

void* Resize_Alloc(void* original_ptr, unsigned long new_size_in_bytes) {
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc) - Legacy memory allocation API
  void* ptr = realloc(original_ptr, new_size_in_bytes);

  if (!ptr && Memory_Error) {
    Memory_Error();
  }

  return ptr;
}

long Ram_Free(MemoryFlagType /*flag*/) { return 64 * 1024 * 1024; }
long Total_Ram_Free(MemoryFlagType /*flag*/) { return 64 * 1024 * 1024; }
long Heap_Size(MemoryFlagType /*flag*/) { return 64 * 1024 * 1024; }
