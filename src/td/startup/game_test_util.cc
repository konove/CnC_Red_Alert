#include "td/startup/game_test_util.h"

#include <string_view>

#include "base/numeric.h"
#include "engine/file/string_table_test_util.h"
#include "td/assets/assets.h"
#include "td/core/defines.h"
#include "td/map/display.h"
#include "td/map/layer.h"
#include "td/map/map.h"
#include "td/map/world.h"

namespace td {

GameFixture::GameFixture() {
  // The map layer alone: the display layers above it load game data.
  TheMap().MapClass::One_Time();
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

}  // namespace td
