#include "td/heap.h"

#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "td/vector_impl.h"
#include "tech/archive.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

template class VectorClass<void*>;
template class DynamicVectorClass<void*>;
template class VectorClass<char>;

namespace {

struct FieldObject {
  int32_t value = 0;
  bool flag = false;
  int coding_calls = 0;

  template <class Archive>
  void Serialize(Archive& ar) {
    ar(value, flag);
  }
  [[maybe_unused]] void Code_Pointers() { ++coding_calls; }
  [[maybe_unused]] void Decode_Pointers() { ++coding_calls; }
};
struct InheritedOnly : FieldObject {
  int32_t extra = 0;
};
static_assert(Serializable<FieldObject>);
static_assert(!RawImage<FieldObject>);
static_assert(!Serializable<InheritedOnly>);

// No member initializers: placement construction must preserve the raw value.
struct RawObject {
  RawObject() : value(17) {}
  explicit RawObject(const NoInitClass&) {}
  virtual ~RawObject() = default;
  virtual int Value() { return value; }
  bool Save(ArchiveWriter& ar) {
    int32_t size = sizeof(*this);
    ar(size);
    ar.Bytes(this, size);
    return true;
  }
  void Code_Pointers() { ++value; }
  void Decode_Pointers() { --value; }
  int32_t value;
};
static_assert(RawImage<RawObject>);
static_assert(!Serializable<RawObject>);

template <class T>
std::array<uint8_t, 256> SaveHeap(TFixedIHeapClass<T>& heap) {
  std::array<uint8_t, 256> bytes{};
  BufferPipe sink(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveWriter writer(sink);
  EXPECT_TRUE(heap.Save(writer));
  return bytes;
}

template <class T>
bool LoadHeap(TFixedIHeapClass<T>& heap, const std::array<uint8_t, 256>& bytes,
              int length = 256) {
  BufferStraw source(bytes.data(), length);
  ArchiveReader reader(source);
  return heap.Load(reader) != 0;
}

TEST(TdHeapTest, FieldObjectsPreserveSparseSlotsAndSkipPointerCoding) {
  TFixedIHeapClass<FieldObject> source;
  source.Set_Heap(4);
  auto* first = new (source.Alloc()) FieldObject();
  auto* hole = new (source.Alloc()) FieldObject();
  auto* last = new (source.Alloc()) FieldObject();
  first->value = 123;
  last->value = -456;
  last->flag = true;
  source.Free(hole);
  source.Code_Pointers();
  source.Decode_Pointers();
  EXPECT_EQ(first->coding_calls, 0);
  EXPECT_EQ(last->coding_calls, 0);
  TFixedIHeapClass<FieldObject> loaded;
  loaded.Set_Heap(4);
  ASSERT_TRUE(LoadHeap(loaded, SaveHeap(source), 22));
  ASSERT_EQ(loaded.Count(), 2);
  EXPECT_EQ(loaded.ID(loaded.Ptr(0)), 0);
  EXPECT_EQ(loaded.ID(loaded.Ptr(1)), 2);
  EXPECT_EQ(loaded.Ptr(0)->value, 123);
  EXPECT_EQ(loaded.Ptr(1)->value, -456);
  EXPECT_TRUE(loaded.Ptr(1)->flag);
}

TEST(TdHeapTest, RawFallbackPreservesBytesAndRepairsVtable) {
  TFixedIHeapClass<RawObject> source;
  source.Set_Heap(2);
  auto* object = new (source.Alloc()) RawObject();
  source.Code_Pointers();
  EXPECT_EQ(object->value, 18);
  source.Decode_Pointers();
  auto bytes = SaveHeap(source);
  // count + index + raw size precede the object's vtable pointer.
  for (int i = 0; i < static_cast<int>(sizeof(void*)); ++i) {
    bytes[12 + i] = 0;
  }
  TFixedIHeapClass<RawObject> loaded;
  loaded.Set_Heap(2);
  ASSERT_TRUE(LoadHeap(loaded, bytes));
  EXPECT_EQ(loaded.Ptr(0)->Value(), 17);
}

TEST(TdHeapTest, RejectsNegativeCountsAndOutOfRangeSlots) {
  for (bool invalid_count : {false, true}) {
    std::array<uint8_t, 256> bytes{};
    BufferPipe sink(bytes.data(), 256);
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

TEST(TdHeapTest, RejectsDuplicateSlotsAndTruncatedFields) {
  std::array<uint8_t, 256> bytes{};
  BufferPipe sink(bytes.data(), 256);
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

TEST(TdHeapTest, RejectsWrongRawSizeAndTruncatedRawObjects) {
  TFixedIHeapClass<RawObject> source;
  source.Set_Heap(2);
  new (source.Alloc()) RawObject();
  auto bytes = SaveHeap(source);
  TFixedIHeapClass<RawObject> loaded;
  loaded.Set_Heap(2);
  EXPECT_FALSE(LoadHeap(loaded, bytes, 12 + sizeof(RawObject) - 1));
  loaded.Free_All();
  bytes[8] = 0;
  EXPECT_FALSE(LoadHeap(loaded, bytes));
}

}  // namespace
