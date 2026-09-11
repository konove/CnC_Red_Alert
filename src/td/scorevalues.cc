// Persistent score counters; presentation resources are rebuilt locally.
#include "td/score.h"
#include "tech/archive.h"

template <class Archive>
void ScoreClass::Serialize(Archive& ar) {
  ar.Section(FourCC("SCOR"));
  ar(Score, NKilled, GKilled, CKilled, NBKilled, GBKilled, CBKilled,
     NHarvested, GHarvested, CHarvested, ElapsedTime);
  if constexpr (Archive::kIsReading) {
    ChangingGun = nullptr;
  }
}
template void ScoreClass::Serialize(ArchiveWriter&);
template void ScoreClass::Serialize(ArchiveReader&);
