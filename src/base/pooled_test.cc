// Tests creating and destroying pool-allocated objects with base::NewPooled.

#include "base/pooled.h"

#include <array>
#include <cstddef>

#include "gtest/gtest.h"

namespace {

// A two-slot pool, the shape of the game's object heaps: operator new takes a
// free slot or returns nullptr, operator delete gives the slot back.
struct Slot {
  alignas(std::max_align_t) std::array<std::byte, 64> storage{};
  bool used = false;
};

struct Pool {
  std::array<Slot, 2> slots{};
  int constructed = 0;
  int destroyed = 0;

  [[nodiscard]] int UsedSlots() const {
    int used = 0;
    for (const Slot& slot : slots) {
      used += slot.used ? 1 : 0;
    }
    return used;
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): the pool
// a class-specific operator new reaches; each test starts it empty.
Pool pool;

class Pooled {
 public:
  explicit Pooled(int value) : value_(value) { ++pool.constructed; }
  virtual ~Pooled() { ++pool.destroyed; }

  Pooled(const Pooled&) = delete;
  Pooled& operator=(const Pooled&) = delete;
  Pooled(Pooled&&) = delete;
  Pooled& operator=(Pooled&&) = delete;

  static void* operator new(std::size_t /*size*/) noexcept {
    for (Slot& slot : pool.slots) {
      if (!slot.used) {
        slot.used = true;
        return slot.storage.data();
      }
    }
    return nullptr;
  }

  static void operator delete(void* pointer) {
    for (Slot& slot : pool.slots) {
      if (slot.storage.data() == pointer) {
        slot.used = false;
      }
    }
  }

  [[nodiscard]] int value() const { return value_; }

 private:
  int value_;
};

// A derived pooled class deleted through its base, as the game deletes a unit
// through an ObjectClass pointer.
class DerivedPooled : public Pooled {
 public:
  explicit DerivedPooled(int value) : Pooled(value) {}
};

class PooledTest : public testing::Test {
 protected:
  void SetUp() override { pool = Pool{}; }
  void TearDown() override { EXPECT_EQ(pool.UsedSlots(), 0); }
};

TEST_F(PooledTest, NewPooledConstructsInAPoolSlot) {
  auto* object = base::NewPooled<Pooled>(7);
  ASSERT_NE(object, nullptr);
  EXPECT_EQ(object->value(), 7);
  EXPECT_EQ(pool.UsedSlots(), 1);
  EXPECT_EQ(pool.constructed, 1);

  base::DeletePooled(object);
  EXPECT_EQ(pool.destroyed, 1);
}

TEST_F(PooledTest, NewPooledReturnsNullWhenThePoolIsFull) {
  auto* first = base::NewPooled<Pooled>(1);
  auto* second = base::NewPooled<Pooled>(2);
  auto* third = base::NewPooled<Pooled>(3);
  EXPECT_NE(first, nullptr);
  EXPECT_NE(second, nullptr);
  EXPECT_EQ(third, nullptr);
  EXPECT_EQ(pool.constructed, 2);

  base::DeletePooled(first);
  base::DeletePooled(second);
}

TEST_F(PooledTest, DeletePooledThroughABaseRunsTheDerivedDestructor) {
  Pooled* object = base::NewPooled<DerivedPooled>(4);
  ASSERT_NE(object, nullptr);
  base::DeletePooled(object);
  EXPECT_EQ(pool.destroyed, 1);
}

TEST_F(PooledTest, DeletePooledIgnoresNull) {
  base::DeletePooled(static_cast<Pooled*>(nullptr));
  EXPECT_EQ(pool.destroyed, 0);
}

}  // namespace
