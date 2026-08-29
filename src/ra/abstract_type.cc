#include <cstring>
#include <iterator>

#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/type.h"

AbstractTypeClass::AbstractTypeClass(const RTTIType rtti, const int id,
                                     const int name, const char* ini)
    : RTTI(rtti), ID(id), FullName(name) {
  strncpy(IniName, ini, sizeof(IniName));
  IniName[sizeof(IniName) - 1] = '\0';
}

COORDINATE AbstractTypeClass::Coord_Fixup(const COORDINATE coord) const {
  return coord;
}

int AbstractTypeClass::Full_Name() const {
  // Scenario-specific overrides are matched by a composite key encoding the
  // object type and ID. A negative return signals the caller to look up the
  // string in NameOverride rather than the normal text table.
  for (base::ssize index = 0; index < std::ssize(NameOverride); index++) {
    if (NameIDOverride[index] == (RTTI + 1) * 100 + ID) {
      return static_cast<int>(-(index + 1));
    }
  }
  return FullName;
}

int AbstractTypeClass::Get_Ownable() const {
  return HOUSEF_ALLIES | HOUSEF_SOVIET | HOUSEF_OTHERS;
}
