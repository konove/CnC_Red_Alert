/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\sidebar.h_v   2.18   16 Oct 1995 16:45:24
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_SIDEBAR_H_
#define CNC_RED_ALERT_TD_SIDEBAR_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstdint>

#include "sdllib/keyboard.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/gadget.h"
#include "td/power.h"
#include "td/shapebtn.h"
#include "td/stage.h"
#include "tech/file.h"

class InitClass {};

class SidebarClass : public PowerClass {
 public:
  void ResetTransientUiState() override;
  // Saved gameplay state; runtime UI resources remain local.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	These constants are used to control the sidebar rendering. They are
  *instantiated *	as enumerations since C++ cannot use "const" in this
  *context.
  */
  int SideX = 0;         // x position for side bar
  int SideY = 0;         // y position for side bar
  int SideBarWidth = 0;  // width of the sidebar
  int SideWidth = 0;     // width of the sidebar
  int SideHeight = 0;    // height of the sidebar
  int TopHeight = 0;     // height of top of sidebar
  int MaxVisible = 0;    // max production icons visible
  int ButtonOneWidth = 0;  // Button width.
  int ButtonTwoWidth = 0;  // Button width.
  int ButtonThreeWidth = 0;  // Button width.
  int ButtonHeight = 0;  // Button width.

  static constexpr int kButtonActivator = 100;  // Button ID for the activator.
  static constexpr int kSidebarwidth = 80;
  static constexpr int kColumns = 2;  // Number of side strips on sidebar.

  SidebarClass();

  /*
  ** Initialization
  */
  void One_Time() override;                         // One-time inits
  void Init_Clear() override;                       // Clears all to known state
  void Init_IO() override;                          // Inits button list
  void Init_Theater(TheaterType theater) override;  // Theater-specific inits

  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool complete) override;
  void Refresh_Cells(CELL cell, const int16_t* list) override;

  bool Abandon_Production(RTTIType type, int factory);
  bool Activate(int control);
  bool Add(RTTIType type, int ID);
  bool Sidebar_Click(KeyNumType& input, int x, int y);
  void Recalc();
  bool Factory_Link(int factory, RTTIType type, int id);

  /*
  **	File I/O.
  */

  /*
  **	Each side strip is managed by this class. It handles all strip specific
  **	actions.
  */
  class StripClass : public StageClass {
    class SelectClass : public ControlClass {
     public:
      SelectClass() noexcept;

      void Set_Owner(StripClass& strip, int index);
      StripClass* Strip{nullptr};
      int Index{0};

     protected:
      bool Action(unsigned flags, KeyNumType& key) override;
    };

   public:
    int ObjectWidth = 0;
    int ObjectHeight = 0;
    int StripWidth = 0;
    int LeftEdgeOffset = 0;
    int ButtonSpacingOffset = 0;

    StripClass() = default;
    explicit StripClass(const InitClass& /*unused*/);

    bool Add(RTTIType type, int ID);
    bool Abandon_Production(int factory);
    bool Scroll(bool up);
    bool AI(KeyNumType& input, int x, int y);
    void Draw_It(bool complete);
    void One_Time(int id);
    void Init_Clear();
    void Init_IO(int id);
    static void Init_Theater(TheaterType theater);
    bool Recalc();
    void Activate();
    void Deactivate();
    void Flag_To_Redraw();
    bool Factory_Link(int factory, RTTIType type, int id);
    static const void* Get_Special_Cameo(int type);

    /*
    **	File I/O.
    */
    template <class Archive>
    void Serialize(Archive& ar);

    /*
    **	Working numbers used when rendering and processing the side strip.
    */
    static constexpr int kButtonUp = 200;
    static constexpr int kButtonDown = 210;
    static constexpr int kButtonSelect = 220;
    static constexpr int kMaxBuildables =
        30;  // Maximum number of object types in sidebar.
    static constexpr int kObjectHeight =
        24;  // Pixel height of each buildable object.
    static constexpr int kObjectWidth =
        32;  // Pixel width of each buildable object.
    static constexpr int kStripWidth =
        35;  // Width of strip (not counting border lines).
    static constexpr int kMaxVisible =
        4;  // Number of object slots visible at any one time.
    static constexpr int kScrollRate =
        8;  // The pixel jump while scrolling (larger is faster).
    static constexpr int kButtonSpacingOffset = 4;  // spacing info for buttons
    static constexpr int kUpXOffset = 2;  // Scroll up arrow coordinates.
    static constexpr int kUpYOffset = (kMaxVisible * kObjectHeight) + 1;
    static constexpr int kDownXOffset = 18;  // Scroll down arrow coordinates.
    static constexpr int kDownYOffset = (kMaxVisible * kObjectHeight) + 1;
    static constexpr int kButtonWidth = 16;  // Width of the mini-scroll button.
    static constexpr int kButtonHeight =
        12;  // Height of the mini-scroll button.
             // LEFT_EDGE_OFFSET=2,			// Offset from left edge
             // for building shapes.
    static constexpr int kTextXOffset = 18;  // X offset to print "ready" text.
    static constexpr int kTextYOffset = 15;  // Y offset to print "ready" text.
    static constexpr int kTextColor =
        255;  // Color to use for the "Ready" text.
              // kButtonSpacingOffset = 4, // spacing info for buttons
              // LEFT_EDGE_OFFSET=0,			// Offset from left edge
              // for building shapes. kButtonSpacingOffset = 0, // spacing info
              // for buttons

    /*
    **	This is the coordinate of the upper left corner that this side strip
    **	uses for rendering.
    */
    int X = 0;
    int Y = 0;

    /*
    **	This is a unique identifier for the sidebar strip. Using this
    *identifier, *	it is possible to differentiate the button messages that
    *arrive from the *	common input button list.  It >MUST< be equal to the
    *strip's index into
    ** the Column[] array, because the strip uses it to access the stripclass
    ** buttons.
    */
    int ID = 0;

    /*
    **	Shape numbers for the shapes in the STRIP.SHP file.
    */
    static constexpr int kSbBlank =
        0;  // The blank rectangle to use if there are no objects present.
    static constexpr int kSbFrame = 1;

    /*
    **	If this particular side strip needs to be redrawn, then this flag
    **	will be true.
    */
    bool IsToRedraw : 1 = false;

    /*
    **	If construction is in progress (no other objects in this strip can
    **	be started), then this flag will be true. It will be cleared when
    **	the strip is free to start production again.
    */
    bool IsBuilding : 1 = false;

    /*
    **	This controls the sidebar slide direction. If this is true, then the
    *sidebar *	will scroll downward -- revealing previous objects.
    */
    bool IsScrollingDown : 1 = false;

    /*
    **	If the sidebar is scrolling, then this flag is true. Otherwise it is
    *false.
    */
    bool IsScrolling : 1 = false;

    /*
    **	This is the object (sidebar slot) that is flashing. Only one slot can be
    *flashing *	at any one instant. This is usually the result of a click on the
    *slot and construction *	has commenced.
    */
    int Flasher = -1;

    /*
    **	As the sidebar scrolls up and down, this variable holds the index for
    *the topmost *	visible sidebar slot.
    */
    int TopIndex = 0;

    /*
    **	This is the queued scroll direction and amount. The sidebar
    **	will scroll the number of slots indicated by this value. This
    **	value is set according to the scroll buttons.
    */
    int Scroller = 0;

    /*
    **	The sidebar has smooth scrolling. This is the number of pixels the
    *sidebar *	has slide down. Thus, if this value were 5, then there would be
    *5 pixels of *	the TopIndex-1 sidebar object visible. When the Slid
    *value reaches 24, then *	the value resets to zero and the TopIndex is
    *decremented. For sliding in the *	opposite direction, change the
    *IsScrollingDown flag.
    */
    int Slid = 0;

    /*
    **	This is the count of the number of sidebar slots that are active.
    */
    int BuildableCount = 0;

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
    BuildType Buildables[kMaxBuildables]{};

    /*
    **	Pointer to the shape data for small versions of the logos. These are
    *used as *	placeholder pieces on the side bar.
    */
    static const void* LogoShapes;

    /*
    **	This points to the animation sequence of frames used to mark the passage
    *of time *	as an object is undergoing construction.
    */
    static const void* ClockShapes;

    /*
    ** This points to the animation sequence which deals with special
    ** shapes which handle non-production based icons.
    */
    static const void* SpecialShapes[3];

    /*
    **	This is the last theater that the special palette remap table was loaded
    **	for. If the current theater differs from this recorded value, then the
    **	remap tables are reloaded.
    */
    static TheaterType LastTheater;

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
  bool IsSidebarActive : 1 {false};

  /*
  **	This flag tells the rendering system that the sidebar needs to be
  *redrawn.
  */
  bool IsSidebarToRedraw : 1 {true};

  class SBGadgetClass : public GadgetClass {
   public:
    //				SBGadgetClass() : GadgetClass(SIDE_X+8,
    // SIDE_Y, SIDE_WIDTH-1, SIDE_HEIGHT-1, kLeftUp) {};
    SBGadgetClass() noexcept : GadgetClass(0, 0, 0, 0, kLeftUp) {}

   protected:
    bool Action(unsigned flags, KeyNumType& key) override;
  };

  /*
  **	This is the button that is used to collapse and expand the sidebar.
  ** These buttons must be available to derived classes, for Save/Load.
  */
  static ShapeButtonClass Repair;
  static ShapeButtonClass Upgrade;
  static ShapeButtonClass Zoom;
  static SBGadgetClass Background;

  /*
  **	Pointer to the shape data for the sidebar
  */
  static const void* SidebarShape1;
  static const void* SidebarShape2;

 private:
  bool Activate_Repair(int control);
  bool Activate_Upgrade(int control);
  bool Activate_Demolish(int control);
  bool Scroll(bool up, int column);
  static int Which_Column(RTTIType type);

  bool IsRepairActive : 1 {false};
  bool IsUpgradeActive : 1 {false};
  bool IsDemolishActive : 1 {false};
};

extern template void SidebarClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void SidebarClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_SIDEBAR_H_
