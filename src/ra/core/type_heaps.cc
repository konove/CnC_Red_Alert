// How TypeHeaps tears the heaps down.

#include "ra/core/type_heaps.h"

#include "game/core/heap.h"

namespace ra {

TypeHeaps::~TypeHeaps() {
  // The heaps free their buffers without running element destructors, so
  // the type objects are destroyed here: they own their image dimensions and
  // radar icons.
  house_.Destroy_All();
  building_.Destroy_All();
  aircraft_.Destroy_All();
  infantry_.Destroy_All();
  bullet_.Destroy_All();
  anim_.Destroy_All();
  unit_.Destroy_All();
  vessel_.Destroy_All();
  template_.Destroy_All();
  terrain_.Destroy_All();
  overlay_.Destroy_All();
  smudge_.Destroy_All();
}

}  // namespace ra
