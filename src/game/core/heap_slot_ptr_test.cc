// Tests HeapSlotPtr's encoding of a heap pointer as a slot index.

#include "game/core/heap_slot_ptr.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/span_source.h"
#include "game/core/heap.h"
#include "gtest/gtest.h"

namespace game {

namespace {

struct Part {
  int32_t ID = -1;
};

// Collects everything written to it, so a test can inspect the saved bytes.
class RecordingSink : public ByteSink {
 public:
  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return true;
  }
  std::vector<uint8_t> bytes;
};

class HeapSlotPtrTest : public ::testing::Test {
 protected:
  HeapSlotPtrTest() { heap_.Reset(4); }

  Part* Allocate() {
    Part* part = std::construct_at(heap_.Allocate());
    part->ID = heap_.SlotIndexOf(part);
    return part;
  }

  template <class T>
  std::vector<uint8_t> Write(T* pointer) {
    RecordingSink sink;
    // The call operator is non-const; misc-const-correctness misses the call
    // because its argument is template-dependent.
    // NOLINTNEXTLINE(misc-const-correctness)
    ArchiveWriter writer(sink);
    writer(HeapSlotPtr(pointer, heap_));
    return sink.bytes;
  }

  // Reads `bytes` into `pointer` and returns the reader's error, empty on
  // success.
  template <class T>
  std::string Read(const std::vector<uint8_t>& bytes, T*& pointer) {
    SpanSource source(std::as_bytes(std::span(bytes)));
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
  EXPECT_EQ(Write(none), (std::vector<uint8_t>{0xFF, 0xFF, 0xFF, 0xFF}));

  Part* read = Allocate();
  EXPECT_EQ(Read({0xFF, 0xFF, 0xFF, 0xFF}, read), "");
  EXPECT_EQ(read, nullptr);
}

// The slot index as a little-endian int32 is what CCPtr::Serialize wrote,
// which keeps saves from before the CCPtr removal loadable.
TEST_F(HeapSlotPtrTest, PointerIsItsSlotIndex) {
  Allocate();
  Allocate();
  Part* third = Allocate();

  const std::vector<uint8_t> bytes = Write(third);
  EXPECT_EQ(bytes, (std::vector<uint8_t>{2, 0, 0, 0}));

  Part* read = nullptr;
  EXPECT_EQ(Read(bytes, read), "");
  EXPECT_EQ(read, third);
}

TEST_F(HeapSlotPtrTest, ConstPointerRoundTrips) {
  const Part* part = Allocate();
  const std::vector<uint8_t> bytes = Write(part);

  const Part* read = nullptr;
  EXPECT_EQ(Read(bytes, read), "");
  EXPECT_EQ(read, part);
}

// A pointer into a heap that loads later reads as the slot's address.
TEST_F(HeapSlotPtrTest, EmptySlotReadsAsItsAddress) {
  Part* read = nullptr;
  EXPECT_EQ(Read({3, 0, 0, 0}, read), "");
  EXPECT_EQ(read, heap_.Slot(3));
}

TEST_F(HeapSlotPtrTest, IndexOutsideTheHeapFails) {
  Part* past_end = Allocate();
  EXPECT_NE(Read({4, 0, 0, 0}, past_end), "");
  EXPECT_EQ(past_end, nullptr);

  Part* negative = Allocate();
  EXPECT_NE(Read({0xFE, 0xFF, 0xFF, 0xFF}, negative), "");
  EXPECT_EQ(negative, nullptr);
}

}  // namespace

}  // namespace game
