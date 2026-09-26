#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "engine/platform/memflag.h"

void (*Memory_Error)() = nullptr;
void (*Memory_Error_Exit)(char* string) = nullptr;

void Force_VM_Page_In(void* /*buffer*/, int /*length*/) {}

void Mem_Copy(std::span<const std::byte> source, std::span<std::byte> dest,
              size_t bytes_to_copy) {
  base::CopyBytes(dest, source, bytes_to_copy);
}

int64_t Ram_Free(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
int64_t Total_Ram_Free(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
int64_t Heap_Size(MemoryFlagType /*flag*/) { return 64L * 1024 * 1024; }
