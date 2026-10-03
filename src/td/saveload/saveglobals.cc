// Field-wise TD AI base and layer records.
#include <cstdint>
#include <iterator>

#include "base/array.h"
#include "base/numeric.h"
#include "engine/stream/archive.h"
#include "td/ai/base.h"
#include "td/core/defines.h"
#include "td/map/layer.h"
#include "td/saveload/serialize.h"

namespace td {

template <class Archive>
void PrebuiltBase::Serialize(Archive& ar) {
  ar.Section(FourCC("BASE"));
  auto count = static_cast<int32_t>(std::ssize(nodes_));
  ar(house_, count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || house_ < HOUSE_NONE || house_ >= HOUSE_COUNT || count < 0 ||
        count > MAP_CELL_TOTAL) {
      ar.Fail("invalid saved base house or count");
      return;
    }
    nodes_.clear();
    nodes_.reserve(base::ToSize(count));
  }
  for (int32_t i = 0; i < count; ++i) {
    BaseNode node;
    if constexpr (!Archive::kIsReading) {
      node = base::At(nodes_, i);
    }
    ar(node.type, node.coord);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok() || node.type < STRUCT_WEAP || node.type >= STRUCT_COUNT) {
        ar.Fail("invalid saved base building type");
        return;
      }
      nodes_.push_back(node);
    }
  }
}
template void PrebuiltBase::Serialize(ArchiveWriter&);
template void PrebuiltBase::Serialize(ArchiveReader&);

template <class Archive>
void LayerClass::Serialize(Archive& ar) {
  ar.Section(FourCC("LAYR"));
  SerializeObjectList(ar, objects_);
}
template void LayerClass::Serialize(ArchiveWriter&);
template void LayerClass::Serialize(ArchiveReader&);

}  // namespace td
