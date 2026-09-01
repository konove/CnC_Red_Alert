/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***************************************************************************
 * IconListClass -- Exactly like ListClass, but displays an icon as well
 *                  (actually a 'shape' image), left-aligned, covering
 *                  any text that happens to be there...
 *                  Also, I've added the option of making this class
 *                  responsible for the mem alloc. of the strings, and
 *					an automatic limiting of entries to a
 *set maximum.
 *                                                                         *
 * HISTORY:    07/07/1998 ajw : Created, largely in hack mode.             *
 *=========================================================================*/

#ifndef CNC_RED_ALERT_RA_ICONLIST_H_
#define CNC_RED_ALERT_RA_ICONLIST_H_

#include <cstdint>
#include <string>

#include "ra/list.h"
#include "ra/vector.h"

enum ICONKIND {
  ICON_SHAPE = 0,  //	pIcon points to a shape.
  ICON_DIB         //	pIcon points to DIBitmap data.
};

struct FIXEDICON  //	For putting icons in list entries at a specific fixed
                  // offset.
{
  void* pIcon;
  ICONKIND IconKind;
  int xOffset;
  int yOffset;
  int iWidth;
};

// Everything an IconListClass remembers about one line beyond its text.
//
// Owned by the list, one per item, in the same order as the items.
struct IconList_ItemExtras {
  bool bMultiSelected =
      false;  //	True if selected when bMultiSelect is on.

  void* pIcon[3] = {nullptr, nullptr, nullptr};  //	Icons before the text.
  //	Says what kind of image data each pIcon points to.
  ICONKIND IconKind[3] = {ICON_SHAPE, ICON_SHAPE, ICON_SHAPE};

  //	Tooltip help for the item. Empty means the item has no tooltip.
  std::string szHelp;
  //	Extra string carried along with the item. Empty means none was given.
  std::string szExtraData;

  void* pvExtraData = nullptr;  //	Hidden pointer associated with the item.
  //	Color remap for the item's text, or null for the default color.
  RemapControlType* pColorRemap = nullptr;

  FIXEDICON FixedIcon = {};
};

// Each item carries one type-erased pointer for the caller's own use. Some
// callers keep a small integer there rather than an object. Round tripping an
// int through a void* needs a cast the guidelines would rather nobody wrote,
// so it is written once, here, instead of at every such call site.
inline void* AsItemExtraData(int value) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
  return reinterpret_cast<void*>(static_cast<std::intptr_t>(value));
}

inline int ItemExtraDataAsInt(const void* data) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return static_cast<int>(reinterpret_cast<std::intptr_t>(data));
}

class IconListClass : public ListClass {
 public:
  IconListClass(int id, int x, int y, int w, int h, TextPrintType flags,
                const void* up, const void* down,
                bool bResponsibleForStringAlloc = false, int iSelectionType = 1,
                int iMaxItemsSaved = 0);
  //		IconListClass( const IconListClass& list );
  ~IconListClass() override;

  int Add_Item(const char* text) override;
  virtual int Add_Item(const char* text, const char* szHelp, void* pIcon0,
                       ICONKIND IconKind0,
                       const char* szExtraDataString = nullptr,
                       void* pvExtraDataPtr = nullptr,
                       RemapControlType* pColorRemap = nullptr,
                       void* pIcon1 = nullptr, ICONKIND IconKind1 = ICON_SHAPE,
                       void* pIcon2 = nullptr, ICONKIND IconKind2 = ICON_SHAPE,
                       void* pFixedIcon = nullptr,
                       ICONKIND FixedIconKind = ICON_SHAPE, int iXFixedIcon = 0,
                       int iYFixedIcon = 0, int iFixedIconWidth = -1);

  int Add_Item(int text) override;
  virtual int Add_Item(int text, const char* szHelp, void* pIcon0,
                       ICONKIND IconKind0,
                       const char* szExtraDataString = nullptr,
                       void* pvExtraDataPtr = nullptr,
                       RemapControlType* pColorRemap = nullptr,
                       void* pIcon1 = nullptr, ICONKIND IconKind1 = ICON_SHAPE,
                       void* pIcon2 = nullptr, ICONKIND IconKind2 = ICON_SHAPE,
                       void* pFixedIcon = nullptr,
                       ICONKIND FixedIconKind = ICON_SHAPE, int iXFixedIcon = 0,
                       int iYFixedIcon = 0, int iFixedIconWidth = -1);

  //		virtual int Add_Scroll_Bar();
  //		virtual void Bump(int up);
  //		virtual int Count() const {return List.Count();};
  //		virtual int Current_Index() const;
  //		virtual char const * Current_Item() const;
  //		virtual int Draw_Me(int forced);
  //		virtual char const * Get_Item(int index) const;
  //		virtual int Step_Selected_Index(int forward);
  //		virtual void Flag_To_Redraw();

  //		virtual void Peer_To_Peer(unsigned flags, KeyNumType & key,
  // ControlClass & whom);
  void Remove_Item(const char* text) override;
  void Remove_Item(int) override;
  //		virtual int  Remove_Scroll_Bar();
  //		virtual void Set_Selected_Index(int index);
  //		virtual void Set_Selected_Index(char const * text);
  //		virtual void Set_Tabs(int const * tabs);
  //		virtual int  Set_View_Index(int index);
  //		virtual void Step(int up);
  //		virtual void Set_Position(int x, int y);

  /*
  ** These overloaded list routines handle adding/removing the scroll bar
  ** automatically when the list box is added or removed.
  */
  //		virtual LinkClass & Add(LinkClass & object);
  //		virtual LinkClass & Add_Tail(LinkClass & object);
  //		virtual LinkClass & Add_Head(LinkClass & object);
  //		virtual GadgetClass * Remove();

  virtual void Show_Last_Item();
  virtual bool bItemIsMultiSelected(int index) const;
  virtual void MultiSelect(int index, bool bSelect);
  //	The extra string stored with an item, or nullptr when the item has
  //	none and when the index is out of range.
  virtual const char* Get_Item_ExtraDataString(int index) const;
  virtual void Set_Item_ExtraDataString(int index, const char* szNewString);
  virtual void* Get_Item_ExtraDataPtr(int index) const;
  virtual void Set_Item_ExtraDataPtr(int index, void* pNewValue);
  //	The item's tooltip text, or nullptr when it has none and when the
  //	index is out of range.
  const char* Get_Item_Help(int index) const;
  virtual RemapControlType* Get_Item_Color(int index);
  virtual void Set_Item_Color(int index, RemapControlType* pColorRemap);
  virtual const IconList_ItemExtras* Get_ItemExtras(int index) const;
  virtual void Clear();
  virtual int Get_View_Index() { return CurrentTopIndex; }
  bool bScrollBeingDragged() {
    //	Returns true if the scroll bar of the list is being dragged by the user.
    return (GadgetClass::StuckOn == &ScrollGadget);
  }

  virtual int Find(const char* szItemToFind);
  virtual int FindColor(RemapControlType* pColorRemap);

  virtual bool Set_Item(unsigned int index, const char* szText);
  virtual bool Set_Icon(unsigned int index, unsigned int iIconNumber,
                        void* pIcon, ICONKIND IconKind);

  virtual int GetRealWidth();
  virtual void Resize(int x, int y, int w, int h);
  virtual int IndexUnderMouse();
  virtual int OffsetToIndex(int iIndex, int y);

  virtual int SetSelectType(int iSelectTypeNew) {
    //	Provided to enable horrible hacks, mainly involved with dealing with
    // ListClass's inability 	to have no item selected...
    int iSelectTypeOld = iSelectType;
    iSelectType = iSelectTypeNew;
    return iSelectTypeOld;
  }

 protected:
  int Action(unsigned flags, KeyNumType& key) override;
  void Draw_Entry(int index, int x, int y, int width, int selected) override;

  virtual int Add_Item_Detail(const char* szToken, const char* szHelp,
                              void* pIcon0, ICONKIND IconKind0,
                              const char* szExtraDataString, void* pvExtraData,
                              RemapControlType* pColorRemap, void* pIcon1,
                              ICONKIND IconKind1, void* pIcon2,
                              ICONKIND IconKind2, void* pFixedIcon,
                              ICONKIND FixedIconKind, int iXFixedIcon,
                              int iYFixedIcon, int iFixedIconWidth);

  //	The extras for each item, in the same order as the list's text.
  //	ajw stored these as void* because a vector of the real pointer type
  //	"creates hellacious linking problems" in the 1998 compiler. It does not
  //	here.
  DynamicVectorClass<IconList_ItemExtras*> ExtrasList;

  bool bDoAlloc;    //	True if I am responsible for mem. allocation/deletion of
                    // strings.
                    //		bool bMultiSelect;	//	True if we are
                    // using the multiple item selection feature.
  int iSelectType;  //	0 for no selection shown, 1 for normal ListClass
                    // selection, 2 for n multiple selections
  int iMaxItems;    //	Number of items to limit list to, if bDoAlloc is true.
};

#endif  // CNC_RED_ALERT_RA_ICONLIST_H_
