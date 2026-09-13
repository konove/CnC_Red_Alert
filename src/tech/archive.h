#ifndef CNC_RED_ALERT_TECH_ARCHIVE_H_
#define CNC_RED_ALERT_TECH_ARCHIVE_H_

// Binary archive over the Pipe/Straw chain, used for saved games.
//
// A class makes itself serializable by declaring one member template that
// lists its fields once for both directions:
//
//   template <class Archive>
//   void Serialize(Archive& ar) {
//     Base::Serialize(ar);
//     ar(Strength, Facing, Cargo);
//   }
//
// ArchiveWriter walks the same list to write and ArchiveReader to read, so a
// field cannot be written at one width and read at another. On disk every
// integer is little-endian at its own width, every enum is an int32_t, and a
// bool is one byte, which makes the format identical across platforms.
//
// The archives are unbuffered: raw Pipe::Put and Straw::Get calls may be
// interleaved with archive calls on the same chain.

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "tech/pipe.h"
#include "tech/straw.h"

// Integers and enums the archive writes directly, each at its own width.
// `long` cannot be rejected here because int64_t is `long` on LP64;
// clang-tidy's google-runtime-int keeps `long` members out of Serialize() lists
// instead.
template <class T>
concept ArchiveScalar = std::integral<T> || std::is_enum_v<T>;

// Packs a four-character tag into the uint32_t written by Section().
constexpr uint32_t FourCC(const char (&tag)[5]) {
  return static_cast<uint32_t>(static_cast<unsigned char>(tag[0])) |
         static_cast<uint32_t>(static_cast<unsigned char>(tag[1])) << 8 |
         static_cast<uint32_t>(static_cast<unsigned char>(tag[2])) << 16 |
         static_cast<uint32_t>(static_cast<unsigned char>(tag[3])) << 24;
}

// Shared field dispatch. Derived archives supply Scalar() and Raw(); this
// class turns any supported field type into calls on those two.
template <class Derived>
class ArchiveBase {
 public:
  // Serializes each argument in order. Accepts lvalue members and rvalue
  // proxy objects alike.
  template <class... Ts>
  void operator()(Ts&&... fields) {
    (Field(std::forward<Ts>(fields)), ...);
  }

 private:
  friend Derived;
  ArchiveBase() = default;

  Derived& self() { return static_cast<Derived&>(*this); }

  // Rvalues are proxies: temporaries whose Serialize() stands in for a
  // member that cannot be written directly, such as a coded pointer.
  template <class T>
    requires(!std::is_lvalue_reference_v<T>)
  void Field(T&& proxy) {
    std::forward<T>(proxy).Serialize(self());
  }

  template <class T>
  void Field(T& value) {
    using V = std::remove_cv_t<T>;
    if constexpr (std::is_array_v<V>) {
      using E = std::remove_all_extents_t<V>;
      if constexpr (sizeof(E) == 1 && ArchiveScalar<E>) {
        // Byte arrays, including char[] strings, go through in one call.
        self().Raw(&value, static_cast<int>(sizeof(V)));
      } else {
        for (auto& element : value) {
          Field(element);
        }
      }
    } else if constexpr (std::same_as<V, bool>) {
      uint8_t byte = 0;
      if constexpr (!Derived::kIsReading) {
        byte = value ? 1 : 0;
      }
      self().Scalar(byte);
      if constexpr (Derived::kIsReading) {
        value = byte != 0;
      }
    } else if constexpr (std::is_enum_v<V>) {
      // The value is only read when writing; on the read side it may still
      // be uninitialized storage.
      int32_t raw = 0;
      if constexpr (!Derived::kIsReading) {
        raw = static_cast<int32_t>(value);
      }
      self().Scalar(raw);
      if constexpr (Derived::kIsReading) {
        value = static_cast<V>(raw);
      }
    } else if constexpr (ArchiveScalar<V>) {
      self().Scalar(value);
    } else {
      value.Serialize(self());
    }
  }
};

// Writes fields to a Pipe.
class ArchiveWriter : public ArchiveBase<ArchiveWriter> {
 public:
  static constexpr bool kIsReading = false;

  explicit ArchiveWriter(Pipe& sink ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : sink_(sink) {}

  // Writes a four-byte marker that the reader checks with Section().
  void Section(uint32_t tag) { Scalar(tag); }

  // Writes bytes verbatim. The escape hatch for data that is not yet
  // field-wise; every use should disappear as the migration completes.
  void Bytes(const void* data, int size) { sink_.Put(data, size); }

  template <ArchiveScalar T>
  void Scalar(const T& value) {
    T little = value;
    if constexpr (std::endian::native == std::endian::big) {
      little = std::byteswap(little);
    }
    sink_.Put(&little, sizeof(little));
  }
  void Raw(const void* data, int size) { sink_.Put(data, size); }

 private:
  Pipe& sink_;
};

// Reads fields from a Straw. The first failure (short read or wrong section
// tag) is recorded and every later read yields zero without touching the
// source, so a corrupt file produces one error instead of a cascade.
class ArchiveReader : public ArchiveBase<ArchiveReader> {
 public:
  static constexpr bool kIsReading = true;

  explicit ArchiveReader(Straw& source ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : source_(source) {}

  // Reads a marker and compares it with the expected tag. Returns false and
  // records an error on mismatch.
  bool Section(uint32_t expected) {
    uint32_t actual = 0;
    Scalar(actual);
    if (ok() && actual != expected) {
      Fail("section tag mismatch");
    }
    return ok();
  }

  // Reads bytes verbatim; see ArchiveWriter::Bytes.
  void Bytes(void* data, int size) { Raw(data, size); }

  [[nodiscard]] bool ok() const { return error_.empty(); }
  [[nodiscard]] std::string_view error() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return error_;
  }

  // Records the first failure. Later reads become no-ops.
  void Fail(std::string_view why) {
    if (error_.empty()) {
      error_ = why;
    }
  }

  template <ArchiveScalar T>
  void Scalar(T& value) {
    T little{};
    Raw(&little, sizeof(little));
    if constexpr (std::endian::native == std::endian::big) {
      little = std::byteswap(little);
    }
    value = little;
  }
  void Raw(void* data, int size) {
    if (!ok()) {
      std::memset(data, 0, static_cast<std::size_t>(size));
      return;
    }
    if (source_.Get(data, size) != size) {
      std::memset(data, 0, static_cast<std::size_t>(size));
      Fail("unexpected end of data");
    }
  }

 private:
  Straw& source_;
  std::string error_;
};

// A type that declares its own Serialize() member template. Inheriting a
// base class's Serialize() is not enough: a derived class that forgot to
// declare one would otherwise be saved as its base and load with every
// derived field missing. The member-pointer test fails for an inherited
// member because its class is the base, not T.
template <class T>
concept Serializable =
    requires(T& t, ArchiveReader& r, ArchiveWriter& w) {
      t.Serialize(r);
      t.Serialize(w);
    } &&
    std::same_as<decltype(&T::template Serialize<ArchiveWriter>),
                 void (T::*)(ArchiveWriter&)>;

#endif  // CNC_RED_ALERT_TECH_ARCHIVE_H_
