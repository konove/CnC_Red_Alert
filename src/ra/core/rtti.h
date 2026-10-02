// Red Alert's run-time type tags: the kind of every game object, type, cell
// and team, as objects report it (AbstractClass::What_Am_I()) and as a Target
// carries it in its high byte.

#ifndef CNC_RED_ALERT_RA_CORE_RTTI_H_
#define CNC_RED_ALERT_RA_CORE_RTTI_H_

namespace ra {

/**********************************************************************
**	These enumerations are used to implement RTTI. The target system
**	uses these and thus there can be no more RTTI types than can fit
**	in the exponent of a target value.
*/
enum class RTTIType {
  RTTI_NONE = 0,
  RTTI_AIRCRAFT,
  RTTI_AIRCRAFTTYPE,
  RTTI_ANIM,
  RTTI_ANIMTYPE,
  RTTI_BUILDING,
  RTTI_BUILDINGTYPE,
  RTTI_BULLET,
  RTTI_BULLETTYPE,
  RTTI_CELL,
  RTTI_FACTORY,
  RTTI_HOUSE,
  RTTI_HOUSETYPE,
  RTTI_INFANTRY,
  RTTI_INFANTRYTYPE,
  RTTI_OVERLAY,
  RTTI_OVERLAYTYPE,
  RTTI_SMUDGE,
  RTTI_SMUDGETYPE,
  RTTI_SPECIAL,
  RTTI_TEAM,
  RTTI_TEAMTYPE,
  RTTI_TEMPLATE,
  RTTI_TEMPLATETYPE,
  RTTI_TERRAIN,
  RTTI_TERRAINTYPE,
  RTTI_TRIGGER,
  RTTI_TRIGGERTYPE,
  RTTI_UNIT,
  RTTI_UNITTYPE,
  RTTI_VESSEL,
  RTTI_VESSELTYPE
};
using enum RTTIType;

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_RTTI_H_
