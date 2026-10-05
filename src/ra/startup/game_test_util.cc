#include "ra/startup/game_test_util.h"

#include <string_view>

#include "base/numeric.h"
#include "engine/file/string_table_test_util.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/assets.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/map/display.h"
#include "ra/map/layer.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/type.h"

namespace ra {

GameFixture::GameFixture() {
  TheTypeHeaps().house().Reset(magic_enum::enum_count<HousesType>());
  TheTypeHeaps().unit().Reset(magic_enum::enum_count<UnitType>());
  HouseTypeClass::Init_Heap();
  UnitTypeClass::Init_Heap();
  TheObjectHeaps().house().Reset(2);
  TheObjectHeaps().unit().Reset(4);
  // The map layer alone: the display layers above it load game data.
  TheMap().MapClass::One_Time();
  // One_Time() only allocates the cells; a cell learns its own number, which
  // finding its neighbours needs, when Init_Cells() builds it in place.
  TheMap().MapClass::Init_Cells();
  TheMap().MapClass::Set_Map_Dimensions(1, 1, MAP_CELL_W - 2, MAP_CELL_H - 2);
}

// Otherwise the layers would keep the objects a test put on the map.
void GameFixture::TearDown() {
  for (LayerClass& layer : DisplayClass::Layer) {
    layer.Clear();
  }
}

void GameFixture::SetText(const int text_id, const std::string_view text) {
  texts_.resize(kTextCount);
  texts_.at(base::ToSize(text_id)) = text;
  // Rebuilt whole: the table is offsets followed by packed strings.
  text_table_ = MakeStringTable(texts_);
  TheAssets().set_system_strings(text_table_);
}

}  // namespace ra
