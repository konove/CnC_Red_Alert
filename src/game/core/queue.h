// File: QueueClass, the fixed-size circular FIFO the games queue network
// events in.

#ifndef CNC_RED_ALERT_GAME_CORE_QUEUE_H_
#define CNC_RED_ALERT_GAME_CORE_QUEUE_H_

#include <array>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "base/array.h"

namespace game {

// A first-in, first-out queue of at most `size` objects, stored in place.
// Objects join at the tail with Add() and leave from the head with Next();
// nothing leaves from the middle. The storage is circular, so neither end
// moves the other objects.
//
// The capacity is part of the behaviour: Add() refuses an object when the
// queue is full, which is how the network code notices an overflowing event
// list. That is why this is not a std::deque.
//
// Example:
//   QueueClass<EventClass, 64> events;
//   if (!events.Add(event)) { /* full: drop it */ }
//   while (events.Count() > 0) {
//     Execute(events.First());
//     events.Next();
//   }
template <class T, int size>
class QueueClass {
  static_assert(size > 0);

 public:
  QueueClass() noexcept = default;

  // Returns the number of objects in the queue. First(), at() and Next()
  // need it to be non-zero.
  [[nodiscard]] int Count() const { return Count_; }

  // Returns the object `index` places from the head: 0 is the oldest and
  // Count() - 1 the newest. Any other index fails in every build mode.
  T& at(int index) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  T& operator[](int index) ABSL_ATTRIBUTE_LIFETIME_BOUND { return at(index); }

  // Returns the oldest object, the head of the line.
  T& First() ABSL_ATTRIBUTE_LIFETIME_BOUND { return base::At(Array, Head); }

  // Empties the queue.
  void Init();

  // Drops the oldest object and returns how many remain. Does nothing on an
  // empty queue.
  int Next();

  // Copies `object` onto the tail. Returns false, leaving the queue as it
  // was, when the queue is full.
  bool Add(const T& object);

 private:
  int Count_ = 0;  // Number of objects in the queue.
  int Head = 0;    // Slot of the oldest object.
  int Tail = 0;    // Slot the next Add() fills.

  std::array<T, size> Array{};
};

template <class T, int size>
void QueueClass<T, size>::Init() {
  Count_ = 0;
  Head = 0;
  Tail = 0;
}

template <class T, int size>
bool QueueClass<T, size>::Add(const T& object) {
  if (Count_ < size) {
    base::At(Array, Tail) = object;
    Tail = (Tail + 1) % size;
    Count_++;
    return true;
  }
  DLOG(WARNING) << "Queue add failed; count " << Count_ << " size " << size
                << " tail " << Tail << " head " << Head;
  return false;
}

template <class T, int size>
int QueueClass<T, size>::Next() {
  if (Count_) {
    Head = (Head + 1) % size;
    Count_--;
  }
  return Count_;
}

// Checks against Count() before wrapping, so an index outside the active
// objects fails instead of landing on a stale slot.
template <class T, int size>
T& QueueClass<T, size>::at(int index) {
  CHECK_GE(index, 0);
  CHECK_LT(index, Count_);
  return base::At(Array, (Head + index) % size);
}

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_QUEUE_H_
