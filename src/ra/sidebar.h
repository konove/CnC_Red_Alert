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

/* $Header: /CounterStrike/SIDEBAR.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SIDEBAR.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : October 20, 1994 *
 *                                                                                             *
 *                  Last Update : October 20, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_SIDEBAR_H_
#define CNC_RED_ALERT_RA_SIDEBAR_H_

#include "ra/control.h"
#include "ra/defines.h"
#include "ra/gadget.h"
#include "ra/power.h"
#include "ra/shapebtn.h"
#include "ra/stage.h"
#include "sdllib/keyboard.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class InitClass {};

class SidebarClass : public PowerClass {
 public:
  // Sidebar geometry in 320x200 pixels.
  static constexpr int kSideX = 320 - 80;  // Upper left corner.
  static constexpr int kSideY = 7 + 70;
  static constexpr int kSideWidth = 80;
  static constexpr int kSideHeight = 200 - (7 + 70);
  static constexpr int kTopHeight =
      13;  // Section with the repair/sell buttons.
  static constexpr int kColumnOneX = 320 - 80 + 8;  // Side strip corners.
  static constexpr int kColumnOneY = kSideY + kTopHeight;
  static constexpr int kColumnTwoX = 320 - 80 + 8 + (80 - 16) / 2 + 3;
  static constexpr int kColumnTwoY = 7 + 70 + 13;
  static constexpr int kColumns = 2;  // Side strips on the sidebar.

  static void* SidebarShape;
  static void* SidebarMiddleShape;  // Only used in Win95 version
  static void* SidebarBottomShape;  // Only used in Win95 version

  SidebarClass();
  SidebarClass(const NoInitClass& x);

  /*
  ** Initialization
  */
  void One_Time() override;                         // One-time inits
  void Init_Clear() override;                       // Clears all to known state
  void Init_IO() override;                          // Inits button list
  void Init_Theater(TheaterType theater) override;  // Theater-specific inits
  void Reload_Sidebar();  // Loads house-specific sidebar art

  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool complete) override;
  void Refresh_Cells(CELL cell, const short* list) override;

  void Zoom_Mode_Control();
  bool Abandon_Production(RTTIType type, int factory);
  bool Activate(int control);
  bool Add(RTTIType type, int ID);
  bool Sidebar_Click(KeyNumType& input, int x, int y);
  void Recalc();
  bool Factory_Link(int factory, RTTIType type, int id);

  /*
  **	Each side strip is managed by this class. It handles all strip specific
  **	actions.
  */
  class StripClass : public StageClass {
    class SelectClass : public ControlClass {
     public:
      SelectClass();
      SelectClass(const NoInitClass& x) : ControlClass(x) {}

      void Set_Owner(StripClass& strip, int index);

      StripClass* Strip;
      int Index;

     protected:
      int Action(unsigned flags, KeyNumType& key) override;
    };

   public:
    StripClass() {}
    StripClass(const InitClass&);
    StripClass(const NoInitClass&) {}

    bool Add(RTTIType type, int ID);
    bool Abandon_Production(int factory);
    bool Scroll(bool up);
    bool AI(KeyNumType& input, int x, int y);
    void Draw_It(bool complete);
    void One_Time(int id);
    void Init_Clear();
    void Init_IO(int id);
    void Init_Theater(TheaterType theater);
    void Reload_LogoShapes();
    bool Recalc();
    void Activate();
    void Deactivate();
    void Flag_To_Redraw();
    bool Factory_Link(int factory, RTTIType type, int id);
    const void* Get_Special_Cameo(SpecialWeaponType type);

    /*
    **	File I/O.
    */
    bool Load(Straw& file);
    bool Save(Pipe& file) const;

    // Button IDs for the strip's gadgets.
    static constexpr int kButtonUp = 200;
    static constexpr int kButtonDown = 210;
    static constexpr int kButtonSelect = 220;

    // Working numbers used when rendering and processing the side strip.
    static constexpr int kMaxBuildables = 75;  // Object types a strip can hold.
    static constexpr int kObjectHeight = 24;   // Pixel size of a buildable
    static constexpr int kObjectWidth = 32;    // object.
    static constexpr int kMaxVisible = 4;      // Object slots visible at once.
    static constexpr int kScrollRate = 12;     // Pixel jump while scrolling.
    static constexpr int kUpXOffset = 2;       // Scroll arrow coordinates.
    static constexpr int kUpYOffset = kMaxVisible * kObjectHeight + 1;
    static constexpr int kDownXOffset = 18;
    static constexpr int kDownYOffset = kUpYOffset;
    static constexpr int kLeftEdgeOffset = 2;  // Building shapes, from the
                                               // left edge.

    /*
    **	This is the coordinate of the upper left corner that this side strip
    **	uses for rendering.
    */
    int X, Y;

    /*
    **	This is a unique identifier for the sidebar strip. Using this
    *identifier, *	it is possible to differentiate the button messages that
    *arrive from the *	common input button list.  It >MUST< be equal to the
    *strip's index into
    ** the Column[] array, because the strip uses it to access the stripclass
    ** buttons.
    */
    int ID;

    /*
    **	Shape numbers for the shapes in the STRIP.SHP file.
    */
    enum SideBarStipShapeEnums {
      SB_BLANK,  // The blank rectangle to use if there are no objects present.
      SB_FRAME
    };

    /*
    **	If this particular side strip needs to be redrawn, then this flag
    **	will be true.
    */
    unsigned IsToRedraw : 1;

    /*
    **	If construction is in progress (no other objects in this strip can
    **	be started), then this flag will be true. It will be cleared when
    **	the strip is free to start production again.
    */
    unsigned IsBuilding : 1;

    /*
    **	This controls the sidebar slide direction. If this is true, then the
    *sidebar *	will scroll downward -- revealing previous objects.
    */
    unsigned IsScrollingDown : 1;

    /*
    **	If the sidebar is scrolling, then this flag is true. Otherwise it is
    *false.
    */
    unsigned IsScrolling : 1;

    /*
    **	This is the object (sidebar slot) that is flashing. Only one slot can be
    *flashing *	at any one instant. This is usually the result of a click on the
    *slot and construction *	has commenced.
    */
    int Flasher;

    /*
    **	As the sidebar scrolls up and down, this variable holds the index for
    *the topmost *	visible sidebar slot.
    */
    int TopIndex;

    /*
    **	This is the queued scroll direction and amount. The sidebar
    **	will scroll the number of slots indicated by this value. This
    **	value is set according to the scroll buttons.
    */
    int Scroller;

    /*
    **	The sidebar has smooth scrolling. This is the number of pixels the
    *sidebar *	has slide down. Thus, if this value were 5, then there would be
    *5 pixels of *	the TopIndex-1 sidebar object visible. When the Slid
    *value reaches 24, then *	the value resets to zero and the TopIndex is
    *decremented. For sliding in the *	opposite direction, change the
    *IsScrollingDown flag.
    */
    int Slid;

    /*
    ** The value of Slid the last time we rendered the sidebar.
    */
    int LastSlid;

    /*
    **	This is the count of the number of sidebar slots that are active.
    */
    int BuildableCount;

    /*
    **	This is the array of buildable object types. This array is sorted in the
    *order *	that it is to be displayed. This array keeps track of which
    *objects are building *	and ready to be placed. The very nature of this
    *method precludes simultaneous *	construction of the same object type.
    */
    typedef struct BuildType {
      int BuildableID;
      RTTIType BuildableType;
      int Factory;  // Production manager.
    } BuildType;
    BuildType Buildables[kMaxBuildables];

    /*
    **	Pointer to the shape data for small versions of the logos. These are
    *used as *	placeholder pieces on the side bar.
    */
    static void* LogoShapes;

    /*
    **	This points to the animation sequence of frames used to mark the passage
    *of time *	as an object is undergoing construction.
    */
    static const void* ClockShapes;

    /*
    ** This points to the animation sequence which deals with special
    ** shapes which handle non-production based icons.
    */
    static const void*
        SpecialShapes[magic_enum::enum_count<SpecialWeaponType>()];

    /*
    **	This is the last theater that the special palette remap table was loaded
    **	for. If the current theater differs from this recorded value, then the
    **	remap tables are reloaded.
    */
    //				static TheaterType LastTheater;

    static ShapeButtonClass UpButton[kColumns];
    static ShapeButtonClass DownButton[kColumns];
    static SelectClass SelectButton[kColumns][kMaxVisible];

    /*
    **	This points to the shapes that are used for the clock overlay. This
    *displays *	progress of construction.
    */
    static char ClockTranslucentTable[(1 + 1) * 256];

  } Column[kColumns];

  /*
  **	If the sidebar is active then this flag is true.
  */
  unsigned IsSidebarActive : 1;

  /*
  **	This flag tells the rendering system that the sidebar needs to be
  *redrawn.
  */
  unsigned IsToRedraw : 1;

  class SBGadgetClass : public GadgetClass {
   public:
    SBGadgetClass()
        : GadgetClass((kSideX + 8) * 2, kSideY * 2, (kSideWidth - 1) * 2 - 1,
                      (kSideHeight - 1) * 2, LEFTUP) {}

   protected:
    int Action(unsigned flags, KeyNumType& key) override;
  };

  /*
  **	This is the button that is used to collapse and expand the sidebar.
  ** These buttons must be available to derived classes, for Save/Load.
  */
  static ShapeButtonClass Repair;
  static ShapeButtonClass Upgrade;
  static ShapeButtonClass Zoom;
  static SBGadgetClass Background;

  bool Scroll(bool up, int column);

 private:
  bool Activate_Repair(int control);
  bool Activate_Upgrade(int control);
  bool Activate_Demolish(int control);
  int Which_Column(RTTIType type);

  unsigned IsRepairActive : 1;
  unsigned IsUpgradeActive : 1;
  unsigned IsDemolishActive : 1;
};

#endif  // CNC_RED_ALERT_RA_SIDEBAR_H_
