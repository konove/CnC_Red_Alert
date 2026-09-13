// Field-wise TD AI base and layer records.
#include <cstdint>

#include "td/base.h"
#include "td/defines.h"
#include "td/layer.h"
#include "td/serialize.h"
#include "tech/archive.h"

template <class Archive>
void BaseClass::Serialize(Archive& ar) {
  ar.Section(FourCC("BASE"));
  int32_t count = static_cast<int32_t>(Nodes.Count());
  ar(House, count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || House < HOUSE_NONE || House >= HOUSE_COUNT ||
        count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid saved base house or count");
      return;
    }
    Nodes.Clear();
  }
  for (int32_t i = 0; i < count; ++i) {
    BaseNodeClass node;
    if constexpr (!Archive::kIsReading) node = Nodes[i];
    ar(node.Type, node.Coord);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok() || node.Type < STRUCT_WEAP || node.Type >= STRUCT_COUNT) {
        ar.Fail("invalid saved base building type");
        return;
      }
      if (!Nodes.Add(node)) {
        ar.Fail("cannot allocate saved base node");
        return;
      }
    }
  }
}
template void BaseClass::Serialize(ArchiveWriter&);
template void BaseClass::Serialize(ArchiveReader&);

template <class Archive>
void LayerClass::Serialize(Archive& ar) {
  ar.Section(FourCC("LAYR"));
  SerializeObjectList(ar, *this);
}
template void LayerClass::Serialize(ArchiveWriter&);
template void LayerClass::Serialize(ArchiveReader&);
