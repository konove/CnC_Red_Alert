#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/memflag.h"

void (*Memory_Error)() = nullptr;
void (*Memory_Error_Exit)(char* string) = nullptr;

void Force_VM_Page_In(void* /*buffer*/, int /*length*/) {}

char* Alloc(const base::ssize bytes_to_alloc, const MemoryFlagType flags) {
  return (flags & MEM_CLEAR) ? new char[base::ToSize(bytes_to_alloc)]()
                             : new char[base::ToSize(bytes_to_alloc)];
}

void Free(const void* pointer) {
  if (pointer) {
    delete[] (char*)pointer;
  }
}

void Mem_Copy(const void* source, void* dest, size_t bytes_to_copy) {
  memcpy(dest, source, bytes_to_copy);
}

void* Resize_Alloc(void* original_ptr, base::ssize new_size_in_bytes) {
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc) - Legacy memory allocation API
  void* ptr = realloc(original_ptr, base::ToSize(new_size_in_bytes));

  if (!ptr && Memory_Error) {
    Memory_Error();
  }

  return ptr;
}

int64_t Ram_Free(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
int64_t Total_Ram_Free(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
int64_t Heap_Size(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
