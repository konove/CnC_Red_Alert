// Tests the object pool: allocation order, slot reuse, the diagnostics, and
// saving and loading a heap, including the ways a saved stream can be
// malformed.

#include "game/core/heap.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace game {

namespace {
// A stand-in for a game object: something with a slot ID and a few fields.
struct Widget {
  int32_t ID = -1;
  int32_t value = 0;
  bool flag = false;

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(ID, value, flag);
  }
};

static_assert(Serializable<Widget>);

// A derived type that only inherits Serialize() must not count: saving it
// as its base would drop every field it adds.
struct Gadget : Widget {
  int32_t extra = 0;
};
static_assert(!Serializable<Gadget>);

// Constructs a Widget in the heap's next slot and records its slot index the
// way game objects do.
Widget* Allocate(ObjectHeap<Widget>& heap, int32_t value) {
  auto* widget = std::construct_at(heap.Allocate());
  widget->ID = heap.SlotIndexOf(widget);
  widget->value = value;
  return widget;
}

std::vector<std::byte> Save(ObjectHeap<Widget>& heap) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  ArchiveWriter writer(sink);
  EXPECT_TRUE(heap.Save(writer));
  return bytes;
}

// The bytes of `values`, each written as a little-endian int32.
std::vector<std::byte> Int32s(std::initializer_list<int32_t> values) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  ArchiveWriter writer(sink);
  for (int32_t value : values) {
    writer(value);
  }
  return bytes;
}

// Loads `bytes` into `heap` and returns the reader's error, empty on success.
// Also checks that Load's result agrees with the reader.
std::string LoadError(ObjectHeap<Widget>& heap,
                      std::span<const std::byte> bytes) {
  SpanSource source(bytes);
  ArchiveReader reader(source);
  const bool loaded = heap.Load(reader);
  EXPECT_EQ(loaded, reader.ok());
  return std::string(reader.error());
}

bool Load(ObjectHeap<Widget>& heap, std::span<const std::byte> bytes) {
  return LoadError(heap, bytes).empty();
}

TEST(HeapTest, FreeKeepsTheOrderOfTheOthers) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  Widget* first = Allocate(heap, 1);
  Widget* second = Allocate(heap, 2);
  Widget* third = Allocate(heap, 3);

  heap.Free(second);

  ASSERT_EQ(heap.Count(), 2);
  EXPECT_EQ(heap.ActiveAt(0), first);
  EXPECT_EQ(heap.ActiveAt(1), third);
}

TEST(HeapTest, FreeReportsWhetherItFreedAnything) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  Widget* widget = Allocate(heap, 1);

  EXPECT_TRUE(heap.Free(widget));
  EXPECT_FALSE(heap.Free(widget));
  EXPECT_EQ(heap.Count(), 0);
}

// ObjectHeap<T>::Free() takes only a T*, but the untyped core can be handed
// anything, and a pointer into the middle of a slot must not free it.
TEST(HeapTest, FreeRejectsAPointerThatIsNotASlot) {
  ObjectHeapBase heap(base::ssize{sizeof(Widget)});
  heap.Reset(4);
  auto* widget = std::construct_at(static_cast<Widget*>(heap.AllocateSlot()));

  EXPECT_EQ(heap.SlotIndexOf(&widget->value), -1);
  EXPECT_FALSE(heap.Free(&widget->value));
  EXPECT_EQ(heap.Count(), 1);
  EXPECT_TRUE(heap.IsAllocated(0));
  EXPECT_EQ(heap.CheckBookkeeping(), "");
}

// Rule loading sizes the type heaps again with the same count while objects
// still point into them, so the storage must stay where it is.
TEST(HeapTest, SetHeapWithTheSameCountKeepsTheStorage) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  Allocate(heap, 1);
  const Widget* slot_zero = heap.Slot(0);

  heap.Reset(4);

  EXPECT_EQ(heap.Slot(0), slot_zero);
  EXPECT_EQ(heap.Count(), 0);
  EXPECT_EQ(heap.FreeCount(), 4);
}

TEST(HeapTest, AllocateAfterFreeAllReusesSlotZeroZeroed) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  Allocate(heap, 7);
  Allocate(heap, 8);

  heap.FreeAll();
  const Widget* again = heap.Allocate();

  EXPECT_EQ(heap.SlotIndexOf(again), 0);
  EXPECT_EQ(again->value, 0);
  EXPECT_EQ(heap.Count(), 1);
}

TEST(HeapTest, SlotAtNamesEverySlotAndNothingOutside) {
  ObjectHeap<Widget> heap;
  heap.Reset(3);
  const Widget* live = Allocate(heap, 7);

  EXPECT_EQ(heap.SlotOrNull(0), live);
  // An empty slot still has its address: the object created there later.
  EXPECT_EQ(heap.SlotOrNull(2), heap.Slot(2));
  EXPECT_EQ(heap.SlotOrNull(-1), nullptr);
  EXPECT_EQ(heap.SlotOrNull(3), nullptr);
}

TEST(HeapDiagnosticsTest, OwnsOnlySlotStartsInsideTheBuffer) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  const Widget* first = Allocate(heap, 1);
  const Widget* second = Allocate(heap, 2);

  EXPECT_TRUE(heap.Owns(first));
  EXPECT_TRUE(heap.Owns(second));
  // Inside the buffer, but not where a slot begins.
  EXPECT_FALSE(heap.Owns(&first->value));
  // SlotIndexOf() happily turns both of these into plausible-looking indices.
  constexpr Widget stack_widget;
  EXPECT_FALSE(heap.Owns(&stack_widget));
  EXPECT_FALSE(heap.Owns(nullptr));
}

TEST(HeapDiagnosticsTest, DescribeTellsTheWaysAPointerCanBeWrong) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  const Widget* live = Allocate(heap, 1);
  Widget* freed = Allocate(heap, 2);
  heap.Free(freed);

  EXPECT_THAT(heap.Describe(live), testing::HasSubstr("slot=0 taken"));
  EXPECT_THAT(heap.Describe(live), testing::HasSubstr("listed=yes"));
  EXPECT_THAT(heap.Describe(freed), testing::HasSubstr("slot=1 FREE"));
  EXPECT_THAT(heap.Describe(freed), testing::HasSubstr("listed=no"));
  EXPECT_THAT(heap.Describe(&live->value),
              testing::HasSubstr("bytes past the boundary"));

  constexpr Widget stack_widget;
  EXPECT_THAT(heap.Describe(&stack_widget),
              testing::HasSubstr("outside the buffer"));

  const ObjectHeap<Widget> unsized;
  EXPECT_THAT(unsized.Describe(live), testing::HasSubstr("heap has no buffer"));
}

TEST(HeapDiagnosticsTest, ValidateReportsAStaleId) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  Allocate(heap, 1);
  Widget* second = Allocate(heap, 2);
  EXPECT_EQ(heap.Validate(), "");

  second->ID = 7;
  EXPECT_THAT(heap.Validate(), testing::HasSubstr("stale ID 7"));
  EXPECT_THAT(heap.Validate(), testing::HasSubstr("slot=1"));
}

TEST(HeapSerializeTest, SparseSlotsRoundTripIntoTheSameSlots) {
  ObjectHeap<Widget> source;
  source.Reset(8);
  const Widget* a = Allocate(source, 10);
  Widget* b = Allocate(source, 20);
  Widget* c = Allocate(source, 30);
  c->flag = true;
  source.Free(b);  // leaves slots 0 and 2 active
  ASSERT_EQ(source.Count(), 2);

  const std::vector<std::byte> bytes = Save(source);
  // count + 2 x (index + ID + value + flag)
  EXPECT_EQ(bytes.size(), 4U + (2 * (4 + 4 + 4 + 1)));

  ObjectHeap<Widget> loaded;
  loaded.Reset(8);
  ASSERT_TRUE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 2);
  EXPECT_EQ(loaded.SlotIndexOf(loaded.ActiveAt(0)), source.SlotIndexOf(a));
  EXPECT_EQ(loaded.SlotIndexOf(loaded.ActiveAt(1)), source.SlotIndexOf(c));
  EXPECT_EQ(loaded.ActiveAt(0)->value, 10);
  EXPECT_EQ(loaded.ActiveAt(1)->value, 30);
  EXPECT_TRUE(loaded.ActiveAt(1)->flag);
  EXPECT_EQ(loaded.ActiveAt(1)->ID, loaded.SlotIndexOf(loaded.ActiveAt(1)));
  EXPECT_EQ(loaded.FreeCount(), 6);
  // The hole the save left is the next slot handed out.
  EXPECT_EQ(loaded.SlotIndexOf(loaded.Allocate()), 1);
}

TEST(HeapSerializeTest, EmptyHeapRoundTrips) {
  ObjectHeap<Widget> source;
  source.Reset(4);
  const std::vector<std::byte> bytes = Save(source);
  EXPECT_EQ(bytes.size(), 4U);

  ObjectHeap<Widget> loaded;
  loaded.Reset(4);
  EXPECT_TRUE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 0);
}

TEST(HeapSerializeTest, TruncatedStreamFails) {
  ObjectHeap<Widget> source;
  source.Reset(4);
  Allocate(source, 1);
  std::vector<std::byte> bytes = Save(source);
  bytes.resize(bytes.size() - 1);

  ObjectHeap<Widget> loaded;
  loaded.Reset(4);
  EXPECT_FALSE(Load(loaded, bytes));
}

TEST(HeapSerializeTest, CountBeyondCapacityFails) {
  ObjectHeap<Widget> source;
  source.Reset(8);
  for (int i = 0; i < 6; i++) {
    Allocate(source, i);
  }
  const std::vector<std::byte> bytes = Save(source);

  ObjectHeap<Widget> smaller;
  smaller.Reset(4);
  EXPECT_EQ(LoadError(smaller, bytes), "invalid saved heap count");
}

TEST(HeapSerializeTest, NegativeCountFails) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  EXPECT_EQ(LoadError(heap, Int32s({-1})), "invalid saved heap count");
}

TEST(HeapSerializeTest, TruncatedCountFails) {
  ObjectHeap<Widget> heap;
  heap.Reset(4);
  const std::vector<std::byte> count = Int32s({1});
  EXPECT_NE(LoadError(heap, std::span(count).first(3)), "");
  EXPECT_EQ(heap.Count(), 0);
}

TEST(HeapSerializeTest, SaveReportsASinkThatFills) {
  ObjectHeap<Widget> heap;
  heap.Reset(2);
  Allocate(heap, 1);
  // Room for the count and the slot index, not for the object's fields.
  std::array<std::byte, 8> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  EXPECT_FALSE(heap.Save(writer));
}

TEST(HeapSerializeTest, SlotIndexOutOfRangeFails) {
  ObjectHeap<Widget> source;
  source.Reset(8);
  const Widget* widget = Allocate(source, 1);
  EXPECT_EQ(source.SlotIndexOf(widget), 0);
  std::vector<std::byte> bytes = Save(source);
  bytes.at(4) =
      std::byte{0x7F};  // slot index low byte: 127 is past an 8-slot heap

  ObjectHeap<Widget> loaded;
  loaded.Reset(8);
  EXPECT_EQ(LoadError(loaded, bytes), "invalid heap slot");
  EXPECT_EQ(loaded.Count(), 0);
}

TEST(HeapSerializeTest, SlotNamedTwiceFails) {
  ObjectHeap<Widget> source;
  source.Reset(8);
  Allocate(source, 1);
  Allocate(source, 2);
  std::vector<std::byte> bytes = Save(source);
  // Past the count and the first 13-byte record: the second object's slot
  // index, 1 -> 0.
  bytes.at(4 + 13) = std::byte{0};

  ObjectHeap<Widget> loaded;
  loaded.Reset(8);
  EXPECT_EQ(LoadError(loaded, bytes), "invalid heap slot");
  EXPECT_EQ(loaded.Count(), 1);
}

// Freeing an object hands its slot straight back to the next allocation, so a
// pointer read after Free addresses whatever was allocated next rather than a
// dead block. That is what makes reading a projectile pointer after a failed
// unlimbo a live-object read instead of a harmless one.
TEST(HeapTest, FreedSlotIsHandedToTheNextAllocation) {
  ObjectHeap<Widget> heap;
  heap.Reset(2);
  Widget* first = Allocate(heap, 4242);
  heap.Free(first);

  const Widget* second = Allocate(heap, -7);

  EXPECT_EQ(second, first);
  EXPECT_EQ(first->value, -7);
}

TEST(HeapTest, IsAllocatedRejectsHolesAndOutOfRangeSlots) {
  ObjectHeap<Widget> heap;
  heap.Reset(3);
  Widget* widget = Allocate(heap, 1);
  EXPECT_TRUE(heap.IsAllocated(0));
  EXPECT_FALSE(heap.IsAllocated(1));
  EXPECT_FALSE(heap.IsAllocated(-1));
  EXPECT_FALSE(heap.IsAllocated(3));
  heap.Free(widget);
  EXPECT_FALSE(heap.IsAllocated(0));
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

TEST(HeapDestroyTest, DestroyAllRunsEveryDestructorAndFreesTheSlots) {
  ObjectHeap<DestructorCounter> heap;
  heap.Reset(3);
  int destroyed = 0;
  std::construct_at(heap.Allocate(), destroyed);
  std::construct_at(heap.Allocate(), destroyed);

  heap.DestroyAll();

  EXPECT_EQ(destroyed, 2);
  EXPECT_EQ(heap.Count(), 0);
  EXPECT_EQ(heap.FreeCount(), 3);
}

}  // namespace

}  // namespace game
