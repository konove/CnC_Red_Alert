#ifndef CNC_RED_ALERT_RA_SAVELOAD_SERIALIZE_H_
#define CNC_RED_ALERT_RA_SAVELOAD_SERIALIZE_H_

// Proxies that let a Serialize() field list include pointers to heap objects.
//
// A pointer is written as the object's Target (heap kind plus slot index) and
// resolved back to the slot's address on read. Slots have fixed addresses, so
// a pointer to an object in a heap that loads later is still correct once
// that heap has loaded; nothing dereferences it before then.
//
// Example:
//   template <class Archive>
//   void FactoryClass::Serialize(Archive& ar) {
//     ar(Balance, ObjectPtr(Object));
//   }

#include <vector>

#include "absl/base/attributes.h"
#include "engine/stream/archive.h"
#include "ra/core/target.h"

namespace ra {

class ObjectClass;
class TechnoTypeClass;

// Saves an ordered list of object Targets. Read only after all object heaps.
template <class Archive>
// Appending invalidates the elements, not the vector the reference names.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void SerializeObjectList(Archive& ar, std::vector<ObjectClass*>& objects);

// Resolves a saved Target to the raw heap slot it names, without looking at
// the slot's contents. Returns nullptr for Target{}. Records an error on
// the reader and returns nullptr for a kind or index outside the heaps.
ObjectClass* ResolveSavedObject(Target target, ArchiveReader& ar);

// Serializes a pointer to a heap object of type T as a Target. T must derive
// from ObjectClass. Inactive objects are written as Target{}.
template <class T>
class ObjectPtr {
 public:
  explicit ObjectPtr(T*& ref ABSL_ATTRIBUTE_LIFETIME_BOUND) : ref_(ref) {}

  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  T*& ref_;
};

template <class T>
ObjectPtr(T*&) -> ObjectPtr<T>;

// Serializes a pointer to a TechnoTypeClass as a Target. The type heaps are
// filled from the rules before a save is loaded, so the pointer is resolved
// directly on read.
class TechnoTypePtr {
 public:
  explicit TechnoTypePtr(
      const TechnoTypeClass*& ref ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : ref_(ref) {}

  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  const TechnoTypeClass*& ref_;
};

class FootClass;
class RadioClass;
class TechnoClass;

// serialize.cc instantiates ObjectPtr for these types.
extern template class ObjectPtr<ObjectClass>;
extern template class ObjectPtr<RadioClass>;
extern template class ObjectPtr<TechnoClass>;
extern template class ObjectPtr<FootClass>;

extern template void SerializeObjectList<ArchiveWriter>(
    ArchiveWriter&, std::vector<ObjectClass*>&);
extern template void SerializeObjectList<ArchiveReader>(
    ArchiveReader&, std::vector<ObjectClass*>&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SAVELOAD_SERIALIZE_H_
