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

/* $Header: /CounterStrike/EXPAND.CPP 7     3/17/97 1:05a Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : EXPAND.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/03/95 *
 *                                                                                             *
 *                  Last Update : Mar 01, 1997 [V.Grippi] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * EListClass::Draw_Entry -- Draws entry for expansion scenario. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/expand.h"

#include <cstdio>

#include "port/safe_string.h"
#include "ra/ccfile.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/palette.h"
#include "ra/profile.h"
#include "ra/scenario.h"
#include "ra/textbtn.h"
#include "ra/wolstrng.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

// #define CS_DEBUG

/***********************************************************************************************
 * Expansion_CS_Present -- Is the Counterstrike expansion available? *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   true if counterstrike installed *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 3/5/97 1:59PM ST : Fixed to check for EXPAND.MIX *
 *=============================================================================================*/
bool Expansion_CS_Present() {
  //	ajw 9/29/98
  return Is_Counterstrike_Installed();
  //	RawFileClass file("EXPAND.MIX");
  //	return(file.Is_Available());
}

/***********************************************************************************************
 * Expansion_AM_Present -- Is the Aftermath expansion available? *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   true if AfterMath is installed *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 7/9/97 1:59PM BG : Fixed to check for EXPAND2.MIX *
 *=============================================================================================*/
bool Expansion_AM_Present() {
  //	ajw 9/29/98
  return Is_Aftermath_Installed();
  //	RawFileClass file("EXPAND2.MIX");
  //	return(file.Is_Available());
}

const char* ExpandNames[] = {"SCG20EA", "SCG21EA", "SCG22EA", "SCG23EA",
                             "SCG24EA", "SCG26EA", "SCG27EA", "SCG28EA",
                             "SCU31EA", "SCU32EA", "SCU33EA", "SCU34EA",
                             "SCU35EA", "SCU36EA", "SCU37EA", "SCU38EA",
                             "SCG43EA",  // Harbor Reclamation
                             "SCG41EA",  // In the nick of time
                             "SCG40EA",  // Caught in the act
                             "SCG42EA",  // Production Disruption
                             "SCG47EA",  // Negotiations
                             "SCG45EA",  // Monster Tank Madness
                             "SCG44EA",  // Time Flies
                             "SCG48EA",  // Absolut MADness
                             "SCG46EA",  // Pawn

                             "SCU43EA",  // Testing Grounds
                             "SCU40EA",  // Shock Therapy
                             "SCU42EA",  // Let's Make a Steal
                             "SCU41EA",  // Test Drive
                             "SCU45EA",  // Don't Drink The Water
                             "SCU44EA",  // Situation Critical
                             "SCU46EA",  // Brothers in Arms
                             "SCU47EA",  // Deus Ex Machina
                             "SCU48EA",  // Grunyev Revolution
                             nullptr};

const char* TestNames2[] = {
    "SCG01EA", "SCG02EA", "SCG03EA", "SCG04EA", "SCG05EA", "SCG06EA",
    "SCG07EA", "SCG08EA", "SCU01EA", "SCU02EA", "SCU03EA", "SCU04EA",
    "SCU05EA", "SCU06EA", "SCU07EA", "SCU08EA", "SCU09EA", nullptr};

// Translated Counterstrike mission names, in scenario order.
static const char* const kGermanMissionNames[] = {
    "Zusammenstoss", "Unter Tage", "Kontrollierte Verbrennung",
    "Griechenland 1 - Stavros", "Griechenland 2 - Evakuierung",
    "Sibirien 1 - Frische Spuren", "Sibirien 2 - In der Falle",
    "Sibirien 3 - Wildnis", "Das Feld der Ehre", "Belagerung", "Mausefalle",
    "Teslas Erbe", "Soldat Volkov", "Die Spitze der Welt", "Paradoxe Gleichung",
    "Nukleare Eskalation",
    "Ein sicherer Hafen",             //	"SCG43EA",		// Harbor Reclamation
    "Zeitkritische Routine",          //	"SCG41EA",		// In the nick
                                      // of time
    "Auf frischer Tat ertappt",       //	"SCG40EA",		// Caught in the
                                      // act
    "Drastischer Baustopp",           //	"SCG42EA",		// Production Disruption
    "Harte Verhandlungen",            //	"SCG47EA",		// Negotiations
    "Ferngelenktes Kriegsspielzeug",  //	"SCG45EA",		//
                                      // Monster Tank Madness
    "Licht aus",                      //	"SCG44EA",		// Time Flies
    "Molekulare Kriegsführung",       //	"SCG48EA",		// Absolut
                                      // MADness
    "Bauernopfer",                    //	"SCG46EA",		// Pawn

    "Testgelände",                  //	"SCU43EA",		// Testing Grounds
    "Schocktherapie",               //	"SCU40EA",		// Shock Therapy
    "Der Letzte seiner Art",        //	"SCU42EA",		// Let's Make a
                                    // Steal
    "Probefahrt",                   //	"SCU41EA",		// Test Drive
    "Schlaftrunk",                  //	"SCU45EA",		// Don't Drink The Water
    "Der jüngste Tag",              //	"SCU44EA",		// Situation Critical
    "Waffenbrüder",                 //	"SCU46EA",		// Brothers in Arms
    "Deus Ex Machina",              //	"SCU47EA",		// Deus Ex Machina
    "Die Replikanten von Grunyev",  //	"SCU48EA",		// Grunyev
                                    // Revolution
    nullptr};

static const char* const kFrenchMissionNames[] = {
    "Gaz Sarin 1: Ravitaillement Fatal",
    "Gaz Sarin 2: En Sous-sol",
    "Gaz Sarin 3: Attaque Chirurgicale",
    "Grèce Occupée 1: Guerre Privée",
    "Grèce Occupée 2: Evacuation",
    "Conflit Sibérien 1: Traces Fraîches",
    "Conflit Sibérien 2: Pris au Piège",
    "Conflit Sibérien 3: Terres de Glace",
    "Mise à l'Epreuve",
    "Assiégés",
    "La Souricière",
    "L'Héritage de Tesla",
    "Tandem de Choc",
    "Jusqu'au Sommet du Monde",
    "Effets Secondaires",
    "Intensification nucléaire",
    "Le vieux port",           //	"SCG43EA",		// Harbor Reclamation
    "Juste à temps",           //	"SCG41EA",		// In the nick of time
    "La main dans le sac",     //	"SCG40EA",		// Caught in the act
    "Production interrompue",  //	"SCG42EA",		// Production
                               // Disruption
    "Négociations",            //	"SCG47EA",		// Negotiations
    "Tanks en folie!",         //	"SCG45EA",		// Monster Tank Madness
    "Le temps passe",          //	"SCG44EA",		// Time Flies
    "Démence absolue",         //	"SCG48EA",		// Absolut MADness
    "Le pion",                 //	"SCG46EA",		// Pawn

    "Terrains d'essais",         //	"SCU43EA",		// Testing Grounds
    "Thérapie de choc",          //	"SCU40EA",		// Shock Therapy
    "Au voleur!",                //	"SCU42EA",		// Let's Make a Steal
    "Essai de conduite",         //	"SCU41EA",		// Test Drive
    "Ne buvez pas la tasse",     //	"SCU45EA",		// Don't Drink
                                 // The Water
    "Situation critique",        //	"SCU44EA",		// Situation Critical
    "Frères d'armes",            //	"SCU46EA",		// Brothers in Arms
    "Deus Ex Machina",           //	"SCU47EA",		// Deus Ex Machina
    "La Révolution de Grunyev",  //	"SCU48EA",		// Grunyev
                                 // Revolution

    nullptr,
};

// The name table for this build's language, indexed by scenario number minus
// kMissionNameOffset. English builds take the name from the INI instead.
[[maybe_unused]] static const char* const* const kTranslatedMissionNames =
    config::kIsGerman ? kGermanMissionNames : kFrenchMissionNames;
inline constexpr int kMissionNameOffset = 20;

#define OPTION_WIDTH 560
#define OPTION_HEIGHT 332
#define OPTION_X ((640 - OPTION_WIDTH) / 2)
#define OPTION_Y (400 - OPTION_HEIGHT) / 2

struct EObjectClass {
  HousesType House;
  int Scenario;
  char Name[128];
  char FullName[128];
};

/*
**	Derived from list class to handle expansion scenario listings. The
*listings *	are recorded as EObjectClass objects. The data contained
*specifies the scenario *	number, side, and text description.
*/
class EListClass : public ListClass {
 public:
  EListClass(int id, int x, int y, int w, int h, TextPrintType flags,
             const void* up, const void* down)
      : ListClass(id, x, y, w, h, flags, up, down) {}

  virtual int Add_Object(EObjectClass* obj) {
    return ListClass::Add_Item((const char*)obj);
  }
  virtual EObjectClass* Get_Object(int index) const {
    return (EObjectClass*)ListClass::Get_Item(index);
  }
  virtual EObjectClass* Current_Object() {
    return (EObjectClass*)ListClass::Current_Item();
  }

 protected:
  void Draw_Entry(int index, int x, int y, int width, int selected) override;

 private:
  int Add_Item(const char* text) override { return ListClass::Add_Item(text); }
  int Add_Item(int text) override { return ListClass::Add_Item(text); }
  const char* Current_Item() const override {
    return ListClass::Current_Item();
  }
  const char* Get_Item(int index) const override {
    return ListClass::Get_Item(index);
  }
};

/***********************************************************************************************
 * EListClass::Draw_Entry -- Draws entry for expansion scenario. *
 *                                                                                             *
 *    This overrides the normal list class draw action so that the scenario name
 *will be       * displayed along with the house name. *
 *                                                                                             *
 * INPUT:   index    -- The index of the entry that should be drawn. *
 *                                                                                             *
 *          x,y      -- Coordinate of upper left region to draw the entry into.
 **
 *                                                                                             *
 *          width    -- Width of region (pixels) to draw the entry. *
 *                                                                                             *
 *          selected -- Is this entry considered selected? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/17/1995 JLB : Created. *
 *=============================================================================================*/
void EListClass::Draw_Entry(int index, int x, int y, int width, int selected) {
  char buffer[128];
  RemapControlType* scheme = Get_Color_Scheme();

  int text = TXT_NONE;
  if (Get_Object(index)->House == HOUSE_GOOD) {
    text = TXT_ALLIES;
  } else {
    text = TXT_SOVIET;
  }
  sprintf(buffer, "%s: %s", Text_String(text), Get_Object(index)->Name);

  TextPrintType flags = TextFlags;

  if (selected) {
    flags = flags | TPF_BRIGHT_COLOR;
    LogicPage->Fill_Rect(x, y, x + width - 1, y + LineHeight - 1, 1);
  } else {
    if (!(flags & TPF_USE_GRAD_PAL)) {
      flags = flags | TPF_MEDIUM_COLOR;
    }
  }

  Conquer_Clip_Text_Print(buffer, x + 100, y, scheme, TBLACK,
                          flags & ~TPF_CENTER, width, Tabs);
}

bool Expansion_Dialog(bool bCounterstrike)  //	If not bCounterstrike, then this
                                            // was called for Aftermath.
{
  GadgetClass* buttons = nullptr;

  TextButtonClass ok(200, TXT_OK, kTpfButton, OPTION_X + 40,
                     OPTION_Y + OPTION_HEIGHT - 50);
  TextButtonClass cancel(201, TXT_CANCEL, kTpfButton,
                         OPTION_X + OPTION_WIDTH - 85,
                         OPTION_Y + OPTION_HEIGHT - 50);

  EListClass list(202, OPTION_X + 35, OPTION_Y + 30, OPTION_WIDTH - 70,
                  OPTION_HEIGHT - 85, kTpfButton, MFCD::Retrieve("BTN-UP.SHP"),
                  MFCD::Retrieve("BTN-DN.SHP"));
  buttons = &ok;
  cancel.Add(*buttons);
  list.Add(*buttons);

  /*
  **	Add in all the expansion scenarios.
  */
  CCFileClass file;
  char buffer[128], buffer2[128];
  char* sbuffer = _ShapeBuffer;
  for (int index = 20; index < 36 + 18; index++) {
#ifndef CS_DEBUG
    port::SafeCopy(buffer, ExpandNames[index - 20]);
    port::SafeCopy(buffer2, ExpandNames[index - 20]);
#else
    port::SafeCopy(buffer, TestNames2[index]);
    port::SafeCopy(buffer2, TestNames2[index]);
#endif
    if (buffer[0] == 0) {
      break;
    }

    port::SafeAppend(buffer, ".INI");
    port::SafeAppend(buffer2, ".INI");
    Scen.Set_Scenario_Name(buffer);
    Scen.Scenario = index;
    file.Set_Name(buffer);
    bool bOk;
    if (index < 36) {
      bOk = bCounterstrike;
    } else {
      bOk = !bCounterstrike;
    }

    if (bOk && file.Is_Available()) {
      EObjectClass* obj = new EObjectClass;
      switch (buffer[2]) {
        case 'G':
        case 'g':
          file.Read(sbuffer, 2000);
          sbuffer[2000] = '\r';
          sbuffer[2000 + 1] = '\n';
          sbuffer[2000 + 2] = '\0';
          WWGetPrivateProfileString("Basic", "Name", "x", buffer,
                                    sizeof(buffer), sbuffer);
          if constexpr (config::kIsEnglish) {
            port::SafeCopy(obj->Name, buffer);
          } else {
            port::SafeCopy(obj->Name,
                           kTranslatedMissionNames[index - kMissionNameOffset]);
          }
          port::SafeCopy(obj->FullName, buffer2);
          obj->House = HOUSE_GOOD;
          obj->Scenario = index;
          list.Add_Object(obj);
          break;

        case 'U':
        case 'u':
          file.Read(sbuffer, 2000);
          sbuffer[2000] = '\r';
          sbuffer[2000 + 1] = '\n';
          sbuffer[2000 + 2] = '\0';
          WWGetPrivateProfileString("Basic", "Name", "x", buffer,
                                    sizeof(buffer), sbuffer);
          if constexpr (config::kIsEnglish) {
            port::SafeCopy(obj->Name, buffer);
          } else {
            port::SafeCopy(obj->Name,
                           kTranslatedMissionNames[index - kMissionNameOffset]);
          }
          port::SafeCopy(obj->FullName, buffer2);
          obj->House = HOUSE_BAD;
          obj->Scenario = index;
          list.Add_Object(obj);
          break;

        default:
          delete obj;
          break;
      }
    }
  }

  Set_Logic_Page(SeenBuff);
  bool display = true;
  bool process = true;
  bool okval = true;

  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    Call_Back();

    if (display) {
      display = false;

      Hide_Mouse();

      /*
      **	Load the background picture.
      */
      Load_Title_Page();
      CCPalette.Set();

      Dialog_Box(OPTION_X, OPTION_Y, OPTION_WIDTH, OPTION_HEIGHT);
      if (bCounterstrike) {
        Draw_Caption(TXT_WOL_CS_MISSIONS, OPTION_X, OPTION_Y, OPTION_WIDTH);
      } else {
        Draw_Caption(TXT_WOL_AM_MISSIONS, OPTION_X, OPTION_Y, OPTION_WIDTH);
      }
      buttons->Draw_All();
      Show_Mouse();
    }

    KeyNumType input = buttons->Input();
    switch (input) {
      case ButtonKey(200):
        Whom = list.Current_Object()->House;
        Scen.Scenario = list.Current_Object()->Scenario;
        port::SafeCopy(Scen.ScenarioName, list.Current_Object()->FullName);
        process = false;
        okval = true;
        break;

      case KN_ESC:
      case ButtonKey(201):
        process = false;
        okval = false;
        break;

      case KN_RETURN:
        Whom = list.Current_Object()->House;
        Scen.Scenario = list.Current_Object()->Scenario;
        port::SafeCopy(Scen.ScenarioName, list.Current_Object()->FullName);
        process = false;
        okval = true;
        break;

      default:
        break;
    }
  }

  /*
  **	Free up the allocations for the text lines in the list box.
  */
  for (int index = 0; index < list.Count(); index++) {
    delete list.Get_Object(index);
  }

  return okval;
}
