// How TypeHeaps tears the heaps down.

#include "ra/core/type_heaps.h"

#include "game/core/object_heap.h"

namespace ra {

TypeHeaps::~TypeHeaps() {
  // The heaps free their buffers without running element destructors, so
  // the type objects are destroyed here: they own their image dimensions and
  // radar icons.
  house_.DestroyAll();
  building_.DestroyAll();
  aircraft_.DestroyAll();
  infantry_.DestroyAll();
  bullet_.DestroyAll();
  anim_.DestroyAll();
  unit_.DestroyAll();
  vessel_.DestroyAll();
  template_.DestroyAll();
  terrain_.DestroyAll();
  overlay_.DestroyAll();
  smudge_.DestroyAll();
}

}  // namespace ra
