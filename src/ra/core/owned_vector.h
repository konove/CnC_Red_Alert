// OwnedVector, a list that owns the objects it holds.

#ifndef CNC_RED_ALERT_RA_CORE_OWNED_VECTOR_H_
#define CNC_RED_ALERT_RA_CORE_OWNED_VECTOR_H_

#include <algorithm>
#include <concepts>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "base/array.h"
#include "base/types.h"

namespace ra {

// A list of heap objects that it owns and destroys, for the session's player,
// game, chat, phone book and scenario lists. It keeps the shape of the
// DynamicVectorClass<T*> it replaces -- Count() and at() hand out plain
// pointers -- so code that only reads the list reads it the same way, while
// adding takes a std::unique_ptr and deleting or clearing destroys the object.
//
// Example:
//   OwnedVector<NodeNameType> players;
//   NodeNameType* who = players.Add(std::make_unique<NodeNameType>());
//   players.Delete(who);  // destroys it
template <class T>
class OwnedVector {
 public:
  // Returns the number of objects in the list.
  [[nodiscard]] base::ssize Count() const { return std::ssize(elements_); }

  // Returns the object at index, which must be in range. The list keeps
  // ownership.
  [[nodiscard]] T* at(base::ssize index) {
    return base::At(std::span(elements_), index).get();
  }
  [[nodiscard]] const T* at(base::ssize index) const {
    return base::At(std::span(elements_), index).get();
  }

  // Appends an object, taking ownership, and returns it. `element` must not be
  // null.
  T* Add(std::unique_ptr<T> element) {
    return elements_.emplace_back(std::move(element)).get();
  }

  // Returns the index of an object in the list, or -1 if it is not there.
  [[nodiscard]] base::ssize ID(const T* element) const {
    const auto found = std::ranges::find_if(
        elements_, [element](const auto& e) { return e.get() == element; });
    return found == elements_.end() ? -1 : found - elements_.begin();
  }

  // Removes an object and destroys it, keeping the others in order. Returns
  // false, and does nothing, if it is not in the list. Constrained so that a
  // literal 0 picks the index overload.
  template <class U>
    requires std::same_as<U, T> || std::same_as<U, const T>
  bool Delete(U* element) {
    const base::ssize index = ID(element);
    if (index < 0) {
      return false;
    }
    Delete(index);
    return true;
  }

  // Removes the object at index, which must be in range, and destroys it.
  void Delete(base::ssize index) {
    static_cast<void>(at(index));  // Checks the range.
    elements_.erase(elements_.begin() + index);
  }

  // Destroys every object and empties the list.
  void Clear() { elements_.clear(); }

  // Reorders the list; `less(const T&, const T&)` orders two objects.
  template <class Less>
  void Sort(Less less) {
    std::ranges::sort(elements_, [&less](const auto& a, const auto& b) {
      return less(std::as_const(*a), std::as_const(*b));
    });
  }

 private:
  std::vector<std::unique_ptr<T>> elements_;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_OWNED_VECTOR_H_
