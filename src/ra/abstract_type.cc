#include <cstdint>
#include <cstring>
#include <iterator>

#include "engine/base/array.h"
#include "engine/base/strings/safe_string.h"
#include "engine/base/types.h"
#include "ra/defines.h"
#include "ra/type.h"
#include "ra/world.h"

AbstractTypeClass::AbstractTypeClass(const RTTIType rtti, const int id,
                                     const int name, const char* ini) noexcept
    : RTTI(rtti), ID(id), FullName(name) {
  base::SafeCopy(IniName, ini);
}

COORDINATE AbstractTypeClass::Coord_Fixup(const COORDINATE coord) const {
  return coord;
}

int AbstractTypeClass::Full_Name() const {
  // Scenario-specific overrides are matched by a composite key encoding the
  // object type and ID. A negative return signals the caller to look up the
  // string in NameOverride rather than the normal text table.
  for (base::ssize index = 0; index < std::ssize(TheWorld().name_override());
       index++) {
    if (base::At(std::span(TheWorld().name_override_id()), index) ==
        ((static_cast<int>(RTTI) + 1) * 100) + ID) {
      return static_cast<int>(-(index + 1));
    }
  }
  return FullName;
}

uint32_t AbstractTypeClass::Get_Ownable() const {
  return kHouseFlagAllies | kHouseFlagSoviet | kHouseFlagOthers;
}
