// Tests HeapSlotPtr's encoding of a heap pointer as a slot index.

#include "game/core/heap_slot_ptr.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "game/core/heap.h"
#include "gtest/gtest.h"

namespace game {

namespace {

struct Part {
  int32_t ID = -1;
};

// `values` as bytes, for spelling out an expected encoding.
std::vector<std::byte> Bytes(std::initializer_list<uint8_t> values) {
  std::vector<std::byte> bytes;
  for (const uint8_t value : values) {
    bytes.push_back(std::byte{value});
  }
  return bytes;
}

class HeapSlotPtrTest : public ::testing::Test {
 protected:
  HeapSlotPtrTest() { heap_.Reset(4); }

  Part* Allocate() {
    Part* part = std::construct_at(heap_.Allocate());
    part->ID = heap_.SlotIndexOf(part);
    return part;
  }

  template <class T>
  std::vector<std::byte> Write(T* pointer) {
    std::vector<std::byte> bytes;
    VectorSink sink(bytes);
    // The call operator is non-const; misc-const-correctness misses the call
    // because its argument is template-dependent.
    // NOLINTNEXTLINE(misc-const-correctness)
    ArchiveWriter writer(sink);
    writer(HeapSlotPtr(pointer, heap_));
    return bytes;
  }

  // Reads `bytes` into `pointer` and returns the reader's error, empty on
  // success.
  template <class T>
  std::string Read(std::span<const std::byte> bytes, T*& pointer) {
    SpanSource source(bytes);
    // The call operator is non-const; misc-const-correctness misses the call
    // because its argument is template-dependent.
    // NOLINTNEXTLINE(misc-const-correctness)
    ArchiveReader reader(source);
    reader(HeapSlotPtr(pointer, heap_));
    return std::string(reader.error());
  }

  ObjectHeap<Part> heap_;
};

TEST_F(HeapSlotPtrTest, NullIsMinusOne) {
  Part* none = nullptr;
  EXPECT_EQ(Write(none), Bytes({0xFF, 0xFF, 0xFF, 0xFF}));

  Part* read = Allocate();
  EXPECT_EQ(Read(Bytes({0xFF, 0xFF, 0xFF, 0xFF}), read), "");
  EXPECT_EQ(read, nullptr);
}

// The slot index as a little-endian int32 is what CCPtr::Serialize wrote,
// which keeps saves from before the CCPtr removal loadable.
TEST_F(HeapSlotPtrTest, PointerIsItsSlotIndex) {
  Allocate();
  Allocate();
  Part* third = Allocate();

  const std::vector<std::byte> bytes = Write(third);
  EXPECT_EQ(bytes, Bytes({2, 0, 0, 0}));

  Part* read = nullptr;
  EXPECT_EQ(Read(bytes, read), "");
  EXPECT_EQ(read, third);
}

TEST_F(HeapSlotPtrTest, ConstPointerRoundTrips) {
  const Part* part = Allocate();
  const std::vector<std::byte> bytes = Write(part);

  const Part* read = nullptr;
  EXPECT_EQ(Read(bytes, read), "");
  EXPECT_EQ(read, part);
}

// A pointer into a heap that loads later reads as the slot's address.
TEST_F(HeapSlotPtrTest, EmptySlotReadsAsItsAddress) {
  Part* read = nullptr;
  EXPECT_EQ(Read(Bytes({3, 0, 0, 0}), read), "");
  EXPECT_EQ(read, heap_.Slot(3));
}

TEST_F(HeapSlotPtrTest, IndexOutsideTheHeapFails) {
  Part* past_end = Allocate();
  EXPECT_NE(Read(Bytes({4, 0, 0, 0}), past_end), "");
  EXPECT_EQ(past_end, nullptr);

  Part* negative = Allocate();
  EXPECT_NE(Read(Bytes({0xFE, 0xFF, 0xFF, 0xFF}), negative), "");
  EXPECT_EQ(negative, nullptr);
}

}  // namespace

}  // namespace game
