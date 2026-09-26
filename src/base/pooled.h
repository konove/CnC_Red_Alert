// Creating and destroying objects whose class allocates them from a pool.

#ifndef CNC_RED_ALERT_BASE_POOLED_H_
#define CNC_RED_ALERT_BASE_POOLED_H_

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace base {

// A class that allocates its objects itself: `new T` calls T::operator new,
// which takes a slot from the class's fixed pool.
template <class T>
concept PoolAllocated = requires(std::size_t size) {
  { T::operator new(size) } -> std::same_as<void*>;
};

// Constructs a T in a slot of its class's pool. Returns nullptr, without
// running the constructor, when the pool is full.
//
// The pool owns the object, not the caller: it hands the object out again
// when iterated and takes the slot back in DeletePooled. That is ownership
// cppcoreguidelines-owning-memory cannot see, so this is the one place a pooled
// object is created with a bare `new`. Discarding the result is fine when the
// constructor hands the object to the game, as an animation or a smudge does.
//
// Example:
//   auto* anim = base::NewPooled<AnimClass>(ANIM_FIRE_SMALL, coord);
//   if (anim != nullptr) anim->Attach_To(this);
template <PoolAllocated T, class... Args>
T* NewPooled(Args&&... args) {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): T's pool owns the object.
  return new T(std::forward<Args>(args)...);
}

// Destroys a pooled object and returns its slot to the pool. `object` may point
// to a base class, which must then have a virtual destructor so the object's own
// class's operator delete runs. Does nothing for nullptr.
template <class T>
  requires PoolAllocated<T> || std::has_virtual_destructor_v<T>
void DeletePooled(T* object) {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): T's pool owns the object.
  delete object;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_POOLED_H_
