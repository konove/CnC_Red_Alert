// Tests loading a heap from a fixed-size buffer: sparse slots, the ways a
// stream can be malformed, and slot reuse.

#include "game/core/heap.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace game {

namespace {

struct FieldObject {
  int32_t value = 0;
  bool flag = false;

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(value, flag);
  }
};
struct InheritedOnly : FieldObject {
  int32_t extra = 0;
};
static_assert(Serializable<FieldObject>);
static_assert(!Serializable<InheritedOnly>);

// Saves `heap` into a zero-padded 256-byte buffer.
template <class T>
std::array<uint8_t, 256> SaveHeap(TFixedIHeapClass<T>& heap) {
  std::array<uint8_t, 256> bytes{};
  SpanSink sink(std::as_writable_bytes(std::span(bytes)));
  ArchiveWriter writer(sink);
  EXPECT_TRUE(heap.Save(writer));
  return bytes;
}

// Loads the first `length` bytes of `bytes` into `heap`; returns whether Load
// succeeded.
template <class T>
bool LoadHeap(TFixedIHeapClass<T>& heap, const std::array<uint8_t, 256>& bytes,
              int length = 256) {
  SpanSource source(
      std::as_bytes(std::span(bytes).first(static_cast<std::size_t>(length))));
  ArchiveReader reader(source);
  return heap.Load(reader) != 0;
}

TEST(HeapLoadTest, FieldObjectsPreserveSparseSlots) {
  TFixedIHeapClass<FieldObject> source;
  source.Set_Heap(4);
  auto* first = std::construct_at(source.Alloc());
  auto* hole = std::construct_at(source.Alloc());
  auto* last = std::construct_at(source.Alloc());
  first->value = 123;
  last->value = -456;
  last->flag = true;
  source.Free(hole);
  TFixedIHeapClass<FieldObject> loaded;
  loaded.Set_Heap(4);
  // count + 2 x (index + value + flag)
  ASSERT_TRUE(LoadHeap(loaded, SaveHeap(source), 22));
  ASSERT_EQ(loaded.Count(), 2);
  EXPECT_EQ(loaded.ID(loaded.Ptr(0)), 0);
  EXPECT_EQ(loaded.ID(loaded.Ptr(1)), 2);
  EXPECT_EQ(loaded.Ptr(0)->value, 123);
  EXPECT_EQ(loaded.Ptr(1)->value, -456);
  EXPECT_TRUE(loaded.Ptr(1)->flag);
  EXPECT_EQ(loaded.ID(loaded.Alloc()), 1);  // The saved hole remains reusable.
}

TEST(HeapLoadTest, RejectsNegativeCountsAndOutOfRangeSlots) {
  for (const bool invalid_count : {false, true}) {
    std::array<uint8_t, 256> bytes{};
    SpanSink sink(std::as_writable_bytes(std::span(bytes).first(256)));
    ArchiveWriter writer(sink);
    int32_t count = invalid_count ? -1 : 1;
    int32_t index = 2;
    writer(count, index);
    TFixedIHeapClass<FieldObject> heap;
    heap.Set_Heap(2);
    EXPECT_FALSE(LoadHeap(heap, bytes));
    EXPECT_EQ(heap.Count(), 0);
  }
}

TEST(HeapLoadTest, RejectsDuplicateSlotsAndTruncatedFields) {
  std::array<uint8_t, 256> bytes{};
  SpanSink sink(std::as_writable_bytes(std::span(bytes).first(256)));
  ArchiveWriter writer(sink);
  int32_t count = 2;
  int32_t index = 0;
  FieldObject value;
  writer(count, index, value, index, value);
  TFixedIHeapClass<FieldObject> heap;
  heap.Set_Heap(2);
  EXPECT_FALSE(LoadHeap(heap, bytes));
  EXPECT_EQ(heap.Count(), 1);
  heap.Free_All();
  EXPECT_FALSE(LoadHeap(heap, bytes, 12));  // First object's bool is missing.
}

TEST(HeapLoadTest, RejectsOversizedCountsAndTruncatedHeaders) {
  std::array<uint8_t, 256> bytes{};
  SpanSink sink(std::as_writable_bytes(std::span(bytes).first(256)));
  ArchiveWriter writer(sink);
  int32_t count = 3;
  writer(count);
  TFixedIHeapClass<FieldObject> heap;
  heap.Set_Heap(2);
  EXPECT_FALSE(LoadHeap(heap, bytes));
  EXPECT_FALSE(LoadHeap(heap, bytes, 3));
  EXPECT_EQ(heap.Count(), 0);
}

TEST(HeapLoadTest, EmptyHeapRoundTripsWithoutObjects) {
  TFixedIHeapClass<FieldObject> source;
  source.Set_Heap(2);
  TFixedIHeapClass<FieldObject> loaded;
  loaded.Set_Heap(2);
  EXPECT_TRUE(LoadHeap(loaded, SaveHeap(source), 4));
  EXPECT_EQ(loaded.Count(), 0);
  EXPECT_EQ(loaded.ID(loaded.Alloc()), 0);
}

}  // namespace

// Freeing an object hands its slot straight back to the next allocation, so a
// pointer read after Free addresses whatever was allocated next rather than a
// dead block. That is what makes reading a projectile pointer after a failed
// unlimbo a live-object read instead of a harmless one.
TEST(HeapLoadTest, FreedSlotIsHandedToTheNextAllocation) {
  TFixedIHeapClass<FieldObject> heap;
  heap.Set_Heap(2);
  auto* first = std::construct_at(heap.Alloc());
  first->value = 4242;
  heap.Free(first);

  auto* second = std::construct_at(heap.Alloc());
  second->value = -7;

  EXPECT_EQ(second, first);
  EXPECT_EQ(first->value, -7);
}

// Counts how many times its destructor has run.
class DestructorCounter {
 public:
  explicit DestructorCounter(int& destroyed ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : destroyed_(&destroyed) {}
  ~DestructorCounter() { ++*destroyed_; }
  DestructorCounter(const DestructorCounter&) = delete;
  DestructorCounter& operator=(const DestructorCounter&) = delete;
  DestructorCounter(DestructorCounter&&) = delete;
  DestructorCounter& operator=(DestructorCounter&&) = delete;

 private:
  int* destroyed_;
};

TEST(HeapLoadTest, DestroyAllRunsEveryDestructorAndFreesTheSlots) {
  TFixedIHeapClass<DestructorCounter> heap;
  heap.Set_Heap(3);
  int destroyed = 0;
  std::construct_at(heap.Alloc(), destroyed);
  std::construct_at(heap.Alloc(), destroyed);

  heap.Destroy_All();

  EXPECT_EQ(destroyed, 2);
  EXPECT_EQ(heap.Count(), 0);
  EXPECT_EQ(heap.Avail(), 3);
}

TEST(HeapLoadTest, AllocationQueryRejectsHolesAndOutOfRangeSlots) {
  TFixedIHeapClass<FieldObject> heap;
  heap.Set_Heap(3);
  auto* object = std::construct_at(heap.Alloc());
  EXPECT_TRUE(heap.Is_Allocated(0));
  EXPECT_FALSE(heap.Is_Allocated(1));
  EXPECT_FALSE(heap.Is_Allocated(-1));
  EXPECT_FALSE(heap.Is_Allocated(3));
  heap.Free(object);
  EXPECT_FALSE(heap.Is_Allocated(0));
}

}  // namespace game
