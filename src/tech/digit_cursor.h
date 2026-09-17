// Bounds-carrying cursors for the multi-precision arithmetic's digit walks.
#ifndef CNC_RED_ALERT_TECH_DIGIT_CURSOR_H_
#define CNC_RED_ALERT_TECH_DIGIT_CURSOR_H_

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "base/numeric.h"
#include "port/unaligned.h"

// Preserves the allocation's bounds while a legacy arithmetic loop walks in
// either direction. Reads and writes copy the value representation, so viewing
// 32-bit digits as 16-bit halves does not violate alignment or aliasing rules.
// The -1 position is a reverse-loop sentinel; it can never be dereferenced.
template <typename T>
class DigitCursor {
 public:
  using Value = std::remove_const_t<T>;
  using Byte =
      std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

  class Reference {
   public:
    // The span object may be temporary; its underlying digit storage must
    // survive.
    // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
    explicit Reference(std::span<std::byte> bytes) : bytes_(bytes) {}
    // A digit reference participates in arithmetic exactly like its value.
    // NOLINTNEXTLINE(*-explicit-constructor)
    operator Value() const { return port::ReadUnaligned<Value>(bytes_); }
    Reference(const Reference&) = default;
    Reference(Reference&&) = default;
    ~Reference() = default;
    Reference& operator=(Reference&& other) noexcept {
      return *this = static_cast<Value>(other);
    }
    Reference& operator=(Value value) {
      port::WriteUnaligned(bytes_, value);
      return *this;
    }
    Reference& operator=(const Reference& other) {
      return *this = static_cast<Value>(other);
    }
    Reference& operator+=(Value value) {
      return *this = static_cast<Value>(static_cast<Value>(*this) + value);
    }
    Reference& operator-=(Value value) {
      return *this = static_cast<Value>(static_cast<Value>(*this) - value);
    }
    Reference& operator|=(Value value) {
      return *this = static_cast<Value>(static_cast<Value>(*this) | value);
    }
    Reference& operator&=(Value value) {
      return *this = static_cast<Value>(static_cast<Value>(*this) & value);
    }
    Reference& operator>>=(unsigned value) ABSL_ATTRIBUTE_LIFETIME_BOUND {
      return *this = static_cast<Value>(static_cast<Value>(*this) >> value);
    }
    Reference& operator++() ABSL_ATTRIBUTE_LIFETIME_BOUND {
      return *this += Value{1};
    }
    Value operator--(int) {
      const Value old = *this;
      *this -= Value{1};
      return old;
    }

   private:
    std::span<std::byte> bytes_;
  };

  DigitCursor() = default;
  // Array and span constructors deliberately exclude raw pointer/count pairs.
  template <typename U, std::size_t N>
    requires std::is_same_v<std::remove_const_t<U>, Value> &&
             (std::is_const_v<T> || !std::is_const_v<U>)
  // Array/span views intentionally convert without changing ownership.
  // NOLINTNEXTLINE(*-explicit-constructor)
  DigitCursor(U (&values)[N]) : DigitCursor(std::span(values)) {}
  template <typename U, std::size_t N>
    requires std::is_same_v<std::remove_const_t<U>, Value> &&
             (std::is_const_v<T> || !std::is_const_v<U>)
  // Array/span views intentionally convert without changing ownership.
  // NOLINTNEXTLINE(*-explicit-constructor)
  DigitCursor(std::span<U, N> values) {
    if constexpr (std::is_const_v<T>) {
      bytes_ = std::as_bytes(values);
    } else {
      bytes_ = std::as_writable_bytes(values);
    }
  }
  template <std::size_t N>
  // Array/span views intentionally convert without changing ownership.
  // NOLINTNEXTLINE(*-explicit-constructor)
  DigitCursor(std::array<Value, N>& values) : DigitCursor(std::span(values)) {}
  template <std::size_t N>
    requires std::is_const_v<T>
  // Array/span views intentionally convert without changing ownership.
  // NOLINTNEXTLINE(*-explicit-constructor)
  DigitCursor(const std::array<Value, N>& values)
      : DigitCursor(std::span(values)) {}
  template <typename U>
    requires std::is_const_v<T> && std::is_same_v<U, Value>
  // Array/span views intentionally convert without changing ownership.
  // NOLINTNEXTLINE(*-explicit-constructor)
  DigitCursor(const DigitCursor<U>& other)
      : bytes_(other.bytes_), position_(other.position_) {}

  static DigitCursor FromBytes(std::span<Byte> bytes) {
    CHECK_EQ(bytes.size() % sizeof(Value), 0);
    DigitCursor result;
    result.bytes_ = bytes;
    return result;
  }
  [[nodiscard]] auto operator*() const { return (*this).at(0); }
  // Returns a digit relative to this cursor, checked against the original
  // storage. Negative offsets are valid when they remain inside that storage.
  [[nodiscard]] auto at(std::ptrdiff_t offset) const {
    CHECK_GE(offset, -position_);
    CHECK_LT(offset, Count() - position_);
    const auto index = position_ + offset;
    const auto bytes =
        bytes_.subspan(base::ToSize(index) * sizeof(Value), sizeof(Value));
    if constexpr (std::is_const_v<T>) {
      return port::ReadUnaligned<Value>(bytes);
    } else {
      return Reference(bytes);
    }
  }
  [[nodiscard]] auto operator[](std::ptrdiff_t offset) const {
    return at(offset);
  }
  DigitCursor& operator+=(std::ptrdiff_t count) {
    CHECK_GE(count, -1 - position_);
    CHECK_LE(count, Count() - position_);
    position_ += count;
    return *this;
  }
  DigitCursor& operator-=(std::ptrdiff_t count) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    CHECK_LE(count, position_ + 1);
    CHECK_GE(count, position_ - Count());
    position_ -= count;
    return *this;
  }
  DigitCursor& operator++() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *this += 1; }
  DigitCursor operator++(int) {
    auto previous = *this;
    ++*this;
    return previous;
  }
  DigitCursor& operator--() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *this -= 1; }
  DigitCursor operator--(int) {
    auto previous = *this;
    --*this;
    return previous;
  }
  DigitCursor operator+(std::ptrdiff_t count) const {
    auto result = *this;
    result += count;
    return result;
  }
  DigitCursor operator-(std::ptrdiff_t count) const {
    auto result = *this;
    result -= count;
    return result;
  }
  bool operator==(std::nullptr_t) const { return bytes_.empty(); }
  bool operator==(const DigitCursor& other) const {
    return bytes_.data() == other.bytes_.data() && position_ == other.position_;
  }
  bool operator>(const DigitCursor& other) const {
    CHECK_EQ(bytes_.data(), other.bytes_.data());
    return position_ > other.position_;
  }
  [[nodiscard]] std::span<Byte> bytes() const {
    CHECK_GE(position_, 0);
    return bytes_.subspan(base::ToSize(position_) * sizeof(Value));
  }
  template <typename U>
    requires(std::is_const_v<U> || !std::is_const_v<T>)
  [[nodiscard]] DigitCursor<U> Rebind() const {
    return DigitCursor<U>::FromBytes(bytes());
  }

 private:
  template <typename>
  friend class DigitCursor;
  [[nodiscard]] std::ptrdiff_t Count() const {
    return static_cast<std::ptrdiff_t>(bytes_.size() / sizeof(Value));
  }
  std::span<Byte> bytes_;
  std::ptrdiff_t position_ = 0;
};

#endif  // CNC_RED_ALERT_TECH_DIGIT_CURSOR_H_
