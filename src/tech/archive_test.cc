#include "tech/archive.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/pipe.h"
#include "tech/xstraw.h"

namespace {

// Pipe terminator that appends everything it receives to a vector.
class VectorPipe : public Pipe {
 public:
  base::ssize Put(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes_.push_back(std::to_integer<uint8_t>(byte));
    }
    return std::ssize(data);
  }
  [[nodiscard]] const std::vector<uint8_t>& bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }

 private:
  std::vector<uint8_t> bytes_;
};

// Writes through `fn` and returns the bytes produced.
template <class Fn>
std::vector<uint8_t> WriteWith(Fn fn) {
  VectorPipe pipe;
  ArchiveWriter writer(pipe);
  fn(writer);
  return pipe.bytes();
}

// Reads `bytes` through `fn` and reports whether the reader stayed healthy.
template <class Fn>
bool ReadWith(const std::vector<uint8_t>& bytes, Fn fn) {
  BufferStraw straw(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(straw);
  fn(reader);
  return reader.ok();
}

static_assert(FourCC("RASV") == 0x56534152U);
static_assert(ArchiveScalar<int32_t>);
static_assert(ArchiveScalar<char>);
static_assert(!ArchiveScalar<double>);

TEST(ArchiveTest, IntegersUseTheirOwnWidthLittleEndian) {
  int8_t i8 = -2;
  int16_t i16 = 0x0102;
  int32_t i32 = 0x01020304;
  int64_t i64 = 0x0102030405060708;
  uint16_t u16 = 0xFFFE;
  const auto bytes = WriteWith([&](auto& ar) { ar(i8, i16, i32, i64, u16); });
  const std::vector<uint8_t> expected = {0xFE, 0x02, 0x01, 0x04, 0x03, 0x02,
                                         0x01, 0x08, 0x07, 0x06, 0x05, 0x04,
                                         0x03, 0x02, 0x01, 0xFE, 0xFF};
  EXPECT_EQ(bytes, expected);

  int8_t r8 = 0;
  int16_t r16 = 0;
  int32_t r32 = 0;
  int64_t r64 = 0;
  uint16_t ru16 = 0;
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(r8, r16, r32, r64, ru16); }));
  EXPECT_EQ(r8, i8);
  EXPECT_EQ(r16, i16);
  EXPECT_EQ(r32, i32);
  EXPECT_EQ(r64, i64);
  EXPECT_EQ(ru16, u16);
}

TEST(ArchiveTest, BoolIsOneByte) {
  bool yes = true;
  bool no = false;
  const auto bytes = WriteWith([&](auto& ar) { ar(yes, no); });
  EXPECT_EQ(bytes, (std::vector<uint8_t>{1, 0}));

  bool r1 = false;
  bool r2 = true;
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(r1, r2); }));
  EXPECT_TRUE(r1);
  EXPECT_FALSE(r2);
}

enum class Narrow : uint8_t { kNone = 0, kA = 7 };
enum Wide { kZero = 0, kNegative = -3 };

TEST(ArchiveTest, EnumsAreAlwaysInt32) {
  Narrow narrow = Narrow::kA;
  Wide wide = kNegative;
  const auto bytes = WriteWith([&](auto& ar) { ar(narrow, wide); });
  EXPECT_EQ(bytes, (std::vector<uint8_t>{7, 0, 0, 0, 0xFD, 0xFF, 0xFF, 0xFF}));

  Narrow rn{};
  Wide rw{};
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(rn, rw); }));
  EXPECT_EQ(rn, Narrow::kA);
  EXPECT_EQ(rw, kNegative);
}

TEST(ArchiveTest, CharArraysAreRawBytesAndOtherArraysElementWise) {
  char name[6] = "abc";
  int16_t values[3] = {1, 2, 3};
  const auto bytes = WriteWith([&](auto& ar) { ar(name, values); });
  const std::vector<uint8_t> expected = {'a', 'b', 'c', 0, 0, 0,
                                         1,   0,   2,   0, 3, 0};
  EXPECT_EQ(bytes, expected);

  char rname[6] = {};
  int16_t rvalues[3] = {};
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(rname, rvalues); }));
  EXPECT_STREQ(rname, "abc");
  EXPECT_EQ(rvalues[2], 3);
}

struct Point {
  int32_t x = 0;
  int32_t y = 0;
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(x, y);
  }
};

struct Shape {
  Point corners[2];
  bool filled = false;
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(corners, filled);
  }
};

static_assert(Serializable<Point>);
static_assert(Serializable<Shape>);
static_assert(!Serializable<int>);

TEST(ArchiveTest, NestedTypesAndArraysOfThemRoundTrip) {
  Shape shape;
  shape.corners[0] = {1, 2};
  shape.corners[1] = {-3, 4};
  shape.filled = true;
  const auto bytes = WriteWith([&](auto& ar) { ar(shape); });
  EXPECT_EQ(bytes.size(), 17U);

  Shape read;
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(read); }));
  EXPECT_EQ(read.corners[1].x, -3);
  EXPECT_EQ(read.corners[1].y, 4);
  EXPECT_TRUE(read.filled);
}

// A proxy is a temporary that serializes something it refers to, the way the
// game encodes object pointers as targets.
struct Doubled {
  int32_t& ref;
  template <class Archive>
  void Serialize(Archive& ar) {
    int32_t twice = ref * 2;
    ar(twice);
    if constexpr (Archive::kIsReading) {
      ref = twice / 2;
    }
  }
};

TEST(ArchiveTest, RvalueProxiesAreAccepted) {
  int32_t value = 21;
  const auto bytes = WriteWith([&](auto& ar) { ar(Doubled{value}); });
  EXPECT_EQ(bytes, (std::vector<uint8_t>{42, 0, 0, 0}));

  int32_t read = 0;
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) { ar(Doubled{read}); }));
  EXPECT_EQ(read, 21);
}

TEST(ArchiveTest, ShortReadFailsOnceAndZeroFillsTheRest) {
  const std::vector<uint8_t> bytes = {1, 0};  // half of an int32_t
  BufferStraw straw(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(straw);
  int32_t first = -1;
  int32_t second = -1;
  reader(first, second);
  EXPECT_FALSE(reader.ok());
  EXPECT_EQ(reader.error(), "unexpected end of data");
  EXPECT_EQ(first, 0);
  EXPECT_EQ(second, 0);
}

TEST(ArchiveTest, SectionTagsMatchOrFail) {
  auto bytes = WriteWith([&](auto& ar) {
    ar.Section(FourCC("HOUS"));
    int32_t v = 5;
    ar(v);
  });
  EXPECT_EQ(bytes.size(), 8U);

  {
    BufferStraw straw(std::as_bytes(std::span(bytes)));
    ArchiveReader reader(straw);
    EXPECT_TRUE(reader.Section(FourCC("HOUS")));
    int32_t v = 0;
    reader(v);
    EXPECT_EQ(v, 5);
    EXPECT_TRUE(reader.ok());
  }
  {
    BufferStraw straw(std::as_bytes(std::span(bytes)));
    ArchiveReader reader(straw);
    EXPECT_FALSE(reader.Section(FourCC("TEAM")));
    EXPECT_EQ(reader.error(), "section tag mismatch");
  }
}

TEST(ArchiveTest, FailKeepsTheFirstError) {
  const std::vector<uint8_t> bytes;
  BufferStraw straw(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(straw);
  reader.Fail("first");
  reader.Fail("second");
  int32_t v = 0;
  reader(v);
  EXPECT_EQ(reader.error(), "first");
}

TEST(ArchiveTest, BytesEscapeHatchRoundTrips) {
  const char blob[4] = {'x', 'y', 'z', 'w'};
  const auto bytes =
      WriteWith([&](auto& ar) { ar.Bytes(std::as_bytes(std::span(blob))); });
  EXPECT_EQ(bytes, (std::vector<uint8_t>{'x', 'y', 'z', 'w'}));
  char read[4] = {};
  EXPECT_TRUE(ReadWith(bytes, [&](auto& ar) {
    ar.Bytes(std::as_writable_bytes(std::span(read)));
  }));
  EXPECT_EQ(read[3], 'w');
}

}  // namespace
