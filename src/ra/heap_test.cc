#include "ra/heap.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "gtest/gtest.h"
#include "ra/queue.h"
#include "ra/search.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "tech/archive.h"
#include "tech/byte_sink.h"
#include "tech/span_source.h"

namespace {
TEST(VectorStorageTest, BorrowedStorageRetainsExtentAndDoesNotGrow) {
  int first = 1;
  int second = 2;
  void* storage[2] = {};
  DynamicVectorClass<void*> values(2, storage);
  EXPECT_TRUE(values.Add(&first));
  EXPECT_TRUE(values.Add_Head(&second));
  EXPECT_EQ(values.at(0), &second);
  EXPECT_EQ(values.at(1), &first);
  EXPECT_FALSE(values.Add(nullptr));
  EXPECT_TRUE(values.Resize(4));
  EXPECT_EQ(values.at(0), &second);
  EXPECT_EQ(values.at(1), &first);
  EXPECT_TRUE(values.Add(nullptr));
  EXPECT_EQ(values.Count(), 3);
}

TEST(IndexStorageTest, PreservesEntriesAcrossGrowthSortAndRemoval) {
  IndexClass<int> index;
  for (int i = 30; i >= 0; --i) {
    ASSERT_TRUE(index.Add_Index(i, i * 2));
  }
  EXPECT_EQ(index.Fetch_Index(17), 34);
  EXPECT_TRUE(index.Remove_Index(17));
  EXPECT_FALSE(index.Is_Present(17));
  EXPECT_EQ(index.Fetch_Index(30), 60);
  EXPECT_EQ(index.Fetch_Index(0), 0);
  index.Clear();
  EXPECT_EQ(index.Count(), 0);
  EXPECT_FALSE(index.Is_Present(30));
}

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

Widget* Allocate(TFixedIHeapClass<Widget>& heap, int32_t value) {
  auto* w = new (heap.Alloc()) Widget();
  w->ID = heap.ID(w);
  w->value = value;
  return w;
}

std::vector<uint8_t> Save(const TFixedIHeapClass<Widget>& heap) {
  RecordingSink pipe;
  EXPECT_TRUE(heap.Save(pipe));
  return pipe.bytes;
}

bool Load(TFixedIHeapClass<Widget>& heap, const std::vector<uint8_t>& bytes) {
  SpanSource straw(std::as_bytes(std::span(bytes)));
  return heap.Load(straw);
}

TEST(HeapSerializeTest, SparseSlotsRoundTripIntoTheSameSlots) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(8);
  const Widget* a = Allocate(source, 10);
  Widget* b = Allocate(source, 20);
  Widget* c = Allocate(source, 30);
  c->flag = true;
  source.Free(b);  // leaves slots 0 and 2 active
  ASSERT_EQ(source.Count(), 2);

  const std::vector<uint8_t> bytes = Save(source);
  // count + 2 x (index + ID + value + flag)
  EXPECT_EQ(bytes.size(), 4U + (2 * (4 + 4 + 4 + 1)));

  TFixedIHeapClass<Widget> loaded;
  loaded.Set_Heap(8);
  ASSERT_TRUE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 2);
  EXPECT_EQ(loaded.ID(loaded.Ptr(0)), source.ID(a));
  EXPECT_EQ(loaded.ID(loaded.Ptr(1)), source.ID(c));
  EXPECT_EQ(loaded.Ptr(0)->value, 10);
  EXPECT_EQ(loaded.Ptr(1)->value, 30);
  EXPECT_TRUE(loaded.Ptr(1)->flag);
  EXPECT_EQ(loaded.Ptr(1)->ID, loaded.ID(loaded.Ptr(1)));
  EXPECT_EQ(loaded.Avail(), 6);
}

TEST(HeapSerializeTest, EmptyHeapRoundTrips) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(4);
  const std::vector<uint8_t> bytes = Save(source);
  EXPECT_EQ(bytes.size(), 4U);

  TFixedIHeapClass<Widget> loaded;
  loaded.Set_Heap(4);
  EXPECT_TRUE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 0);
}

TEST(HeapSerializeTest, TruncatedStreamFails) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(4);
  Allocate(source, 1);
  std::vector<uint8_t> bytes = Save(source);
  bytes.resize(bytes.size() - 1);

  TFixedIHeapClass<Widget> loaded;
  loaded.Set_Heap(4);
  EXPECT_FALSE(Load(loaded, bytes));
}

TEST(HeapSerializeTest, CountBeyondCapacityFails) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(8);
  for (int i = 0; i < 6; i++) {
    Allocate(source, i);
  }
  const std::vector<uint8_t> bytes = Save(source);

  TFixedIHeapClass<Widget> smaller;
  smaller.Set_Heap(4);
  EXPECT_FALSE(Load(smaller, bytes));
}

TEST(HeapSerializeTest, SlotIndexOutOfRangeFails) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(8);
  const Widget* w = Allocate(source, 1);
  EXPECT_EQ(source.ID(w), 0);
  std::vector<uint8_t> bytes = Save(source);
  bytes.at(4) = 0x7F;  // slot index low byte: 127 is past an 8-slot heap

  TFixedIHeapClass<Widget> loaded;
  loaded.Set_Heap(8);
  EXPECT_FALSE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 0);
}

}  // namespace

namespace {

TEST(RaCheckedContainerTest, VectorAccessPreservesConstnessAndCapacity) {
  VectorClass<int> values(2);
  values.at(1) = 42;
  const VectorClass<int>& view = values;
  EXPECT_EQ(view.at(1), 42);
  EXPECT_EQ(&view.at(1), &values.at(1));
}

TEST(RaCheckedContainerTest, QueueAccessFollowsWrappedLogicalOrder) {
  QueueClass<int, 3> queue;
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  ASSERT_TRUE(queue.Add(30));
  ASSERT_TRUE(queue.Add(40));
  EXPECT_EQ(queue.at(0), 20);
  EXPECT_EQ(queue.at(1), 30);
  EXPECT_EQ(queue.at(2), 40);
  queue.at(1) = 31;
  EXPECT_EQ(queue.at(1), 31);
}

TEST(RaCheckedContainerDeathTest, RejectsInvalidVectorAndQueueIndices) {
  VectorClass<int> values(2);
  const VectorClass<int>& view = values;
  QueueClass<int, 3> queue;
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)values.at(-1), "Check failed");
  EXPECT_DEATH((void)view.at(2), "Check failed");
  EXPECT_DEATH((void)queue.at(0), "Check failed");
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  // Both indices formerly wrapped to valid storage outside the active queue.
  EXPECT_DEATH((void)queue.at(-1), "Check failed");
  EXPECT_DEATH((void)queue.at(1), "Check failed");
  EXPECT_DEATH((void)queue.at(4), "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

}  // namespace
