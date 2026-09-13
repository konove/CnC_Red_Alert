#include "ra/heap.h"

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "tech/archive.h"
#include "tech/pipe.h"
#include "tech/xstraw.h"

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

class VectorPipe : public Pipe {
 public:
  int Put(const void* source, int slen) override {
    const auto* begin = static_cast<const uint8_t*>(source);
    bytes.insert(bytes.end(), begin, begin + slen);
    return slen;
  }
  std::vector<uint8_t> bytes;
};

Widget* Allocate(TFixedIHeapClass<Widget>& heap, int32_t value) {
  Widget* w = new (heap.Alloc()) Widget();
  w->ID = heap.ID(w);
  w->value = value;
  return w;
}

std::vector<uint8_t> Save(TFixedIHeapClass<Widget>& heap) {
  VectorPipe pipe;
  EXPECT_TRUE(heap.Save(pipe));
  return pipe.bytes;
}

bool Load(TFixedIHeapClass<Widget>& heap, const std::vector<uint8_t>& bytes) {
  BufferStraw straw(bytes.data(), static_cast<int>(bytes.size()));
  return heap.Load(straw) != 0;
}

TEST(HeapSerializeTest, SparseSlotsRoundTripIntoTheSameSlots) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(8);
  Widget* a = Allocate(source, 10);
  Widget* b = Allocate(source, 20);
  Widget* c = Allocate(source, 30);
  c->flag = true;
  source.Free(b);  // leaves slots 0 and 2 active
  ASSERT_EQ(source.Count(), 2);

  std::vector<uint8_t> bytes = Save(source);
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
  std::vector<uint8_t> bytes = Save(source);
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
  std::vector<uint8_t> bytes = Save(source);

  TFixedIHeapClass<Widget> smaller;
  smaller.Set_Heap(4);
  EXPECT_FALSE(Load(smaller, bytes));
}

TEST(HeapSerializeTest, SlotIndexOutOfRangeFails) {
  TFixedIHeapClass<Widget> source;
  source.Set_Heap(8);
  Widget* w = Allocate(source, 1);
  EXPECT_EQ(source.ID(w), 0);
  std::vector<uint8_t> bytes = Save(source);
  bytes[4] = 0x7F;  // slot index low byte: 127 is past an 8-slot heap

  TFixedIHeapClass<Widget> loaded;
  loaded.Set_Heap(8);
  EXPECT_FALSE(Load(loaded, bytes));
  EXPECT_EQ(loaded.Count(), 0);
}

}  // namespace
