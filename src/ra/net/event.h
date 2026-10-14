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

/* $Header: /CounterStrike/EVENT.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : EVENT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/09/94 *
 *                                                                                             *
 *                  Last Update : December 9, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_NET_EVENT_H_
#define CNC_RED_ALERT_RA_NET_EVENT_H_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/enum_array.h"
#include "base/payload.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/special_flags.h"
#include "ra/core/target.h"

namespace ra {

/*
**	This event class is used to contain all external game events (things
*that the player can *	do at any time) so that these events can be transported
*between linked computers. This *	encapsulation is required in order to
*ensure that each event affects all computers at the *	same time (same game
*frame).
** NOTE: If you add or remove an event type, you must also update the globals
** EventLength[] and EventNames[].
*/
class EventClass {
 public:
  /*
  **	All external events are identified by these labels.
  */
  enum class EventType : uint8_t {
    EMPTY,

    ALLY,           // Make allie of specified house.
    MEGAMISSION,    // Full change of mission with target and destination.
    MEGAMISSION_F,  // Full change of mission with target and destination, and
                    // formation overrides.
    IDLE,           // Request to enter idle mode.
    SCATTER,        // Request to scatter from current location.
    DESTRUCT,       // Self destruct request (surrender action).
    DEPLOY,         // MCV is to deploy at current location.
    PLACE,          // Place building at location specified.
    OPTIONS,        // Bring up options screen.
    GAMESPEED,      // Set game speed
    PRODUCE,        // Start or Resume production.
    SUSPEND,        // Suspend production.
    ABANDON,        // Abandon production.
    PRIMARY,        // Primary factory selected.
    SPECIAL_PLACE,  // Special target location selected
    EXIT,           // Exit game.
    ANIMATION,      // Flash ground as movement feedback.
    REPAIR,         // Repair specified object.
    SELL,           //	Sell specified object.
    SELLCELL,       // Sell wall at specified cell.
    SPECIAL,        // Special options control.

    // Private events.
    FRAMESYNC,  // Game-connection packet; includes Scenario CRC & sender's
                // frame # Used to initiate game connection phase & to
                // reconnect; When one of these is received, the receiver knows
                // there are no associated commands in this packet.
    MESSAGE,    // Message to another player (The message is the 40 bytes
                // after the event class).
    RESPONSE_TIME,  // use a new propagation delay value
    FRAMEINFO,      // Game-heartbeat packet; includes Game CRC & command count
                // All packets sent for a frame are prefixed with one of these
    SAVEGAME,   // allows multiplayer games to save
    ARCHIVE,    // Updates archive target on specified object.
    ADDPLAYER,  // Add a new player

    TIMING,        // new timing values for all systems to use
    PROCESS_TIME,  // a system's average processing time, in ticks per frame

    PROPOSE_DRAW,  // Players proposes that 2-player game be called a
                   // stalemate.
    RETRACT_DRAW,  // Player retracts proposed draw offer.

    LAST_EVENT,  // one past the last event
  };
  using enum EventType;

  // The payload arms: what each kind of event carries after its header. Each
  // names the event types that carry it in Carries(); data<T>() and
  // UpdateData<T>() check that Type is one of them. The arms share the
  // payload's bytes, as the union they replace did, so writing one leaves
  // the bytes past its end as they were.
  struct OptionsData {
    static constexpr bool Carries(EventType type) { return type == SPECIAL; }
    SpecialFlags Data;  // The special option flags.
  };
  struct SellCellData {
    static constexpr bool Carries(EventType type) { return type == SELLCELL; }
    ra::Cell Cell;  // The cell to sell wall at.
  };
  struct TargetData {
    static constexpr bool Carries(EventType type) {
      return type == IDLE || type == SCATTER || type == PRIMARY ||
             type == REPAIR || type == SELL;
    }
    ra::Target Whom;  // The object to apply the event to.
  };
  struct AnimData {
    static constexpr bool Carries(EventType type) { return type == ANIMATION; }
    AnimType What;     // The animation to create.
    HousesType Owner;  // The owner of the animation (when it matters).
    Coordinate Where;  // The location to place the animation.
  };
  struct GeneralData {
    static constexpr bool Carries(EventType type) {
      return type == ALLY || type == GAMESPEED;
    }
    int Value;  // general-purpose data
  };
  // MEGAMISSION_F starts with the same fields, and Execute() reads them
  // through this arm.
  struct MegaMissionData {
    static constexpr bool Carries(EventType type) {
      return type == MEGAMISSION || type == MEGAMISSION_F;
    }
    ra::Target Whom;         // Whom to apply mission to.
    MissionType Mission;     // What mission to apply.
    ra::Target Target;       // Target to assign.
    ra::Target Destination;  // Destination to assign.
  };
  struct MegaMissionFormationData {
    static constexpr bool Carries(EventType type) {
      return type == MEGAMISSION_F;
    }
    ra::Target Whom;         // Whom to apply mission to.
    MissionType Mission;     // What mission to apply.
    ra::Target Target;       // Target to assign.
    ra::Target Destination;  // Destination to assign.
    SpeedType Speed;         // Formation override speed.
    int MaxSpeed;            // Formation override maximum speed.
  };
  // The leading fields of MegaMissionData.
  struct MissionData {
    static constexpr bool Carries(EventType type) {
      return type == MEGAMISSION;
    }
    ra::Target Whom;      // Whom to apply mission to.
    MissionType Mission;  // What mission to apply.
  };
  struct NavComData {
    static constexpr bool Carries(EventType type) { return type == ARCHIVE; }
    ra::Target Whom;   // Whom to apply movement change to.
    ra::Target Where;  // Where to set NavCom to.
  };
  // PLACE comes as a SpecificData from the sidebar, which names only the type
  // and leaves ID -1, and as a PlaceData from the map.
  struct SpecificData {
    static constexpr bool Carries(EventType type) {
      return type == PRODUCE || type == SUSPEND || type == ABANDON ||
             type == PLACE;
    }
    RTTIType Type;
    int ID;
  };
  struct PlaceData {
    static constexpr bool Carries(EventType type) { return type == PLACE; }
    RTTIType Type;
    ra::Cell Cell;
  };
  struct SpecialData {
    static constexpr bool Carries(EventType type) {
      return type == SPECIAL_PLACE;
    }
    int ID;
    ra::Cell Cell;
    // Where in Cell it was placed, as a sub-cell coordinate: the weapons
    // that act on one unit take the one nearest this spot.
    Coordinate Where;
  };
  // FRAMEINFO, FRAMESYNC and RESPONSE_TIME: exactly one of these is sent each
  // frame, whether there is data that frame or not. RESPONSE_TIME sends only
  // Delay.
  struct FrameInfoData {
    static constexpr bool Carries(EventType type) {
      return type == FRAMEINFO || type == FRAMESYNC || type == RESPONSE_TIME;
    }
    uint32_t CRC;  // the game CRC when this packet was generated, to detect
                   // sync errors
    uint16_t CommandCount;  // # commands sent so far, to detect missed packets
    unsigned char Delay;    // propagation delay used this frame
                            // (Frame - Delay = sender's current frame #)
  };
  // The variable-length ADDPLAYER event: Size bytes follow the event on the
  // wire, and Pointer holds them on this side.
  struct VariableData {
    static constexpr bool Carries(EventType type) { return type == ADDPLAYER; }
    void* Pointer;
    uint32_t Size;
  };
  // New timing values for all systems in a multiplayer game; replaces
  // RESPONSE_TIME for the COMM_MULTI_E_COMP protocol.
  struct TimingData {
    static constexpr bool Carries(EventType type) { return type == TIMING; }
    uint16_t DesiredFrameRate;
    uint16_t MaxAhead;
  };
  // Sent by every system to compute the "desired" frame rate for the game.
  struct ProcessTimeData {
    static constexpr bool Carries(EventType type) {
      return type == PROCESS_TIME;
    }
    uint16_t AverageTicks;
  };

  EventType Type = EMPTY;  // Type of queue command object.

  /*
  ** 'Frame' is the frame that the command should execute on.
  ** 27 bits gives over 25 days of playing time without wrapping,
  ** at 30 frames per second, so it should be plenty!
  */
  unsigned Frame : 26 = 0;

  /*
  ** House index of the player originating this event
  */
  unsigned ID : 5 = 0;

  /*
  ** This bit tells us if we've already executed this event.
  */
  unsigned IsExecuted : 1 = 0;


  //-------------- Constructors ---------------------
  // Events cross the wire and are compared as raw bytes, so every byte has to
  // be deterministic - the padding and the unused half of Data included.
  // Member initializers cannot reach padding. Every other defined constructor
  // delegates here to clear IsExecuted and all unused payload bytes.
  EventClass() { base::FillBytes(base::ObjectBytes(*this), 0, sizeof(EventClass)); }
  explicit EventClass(SpecialFlags data);
  EventClass(EventType type, ra::Target target);
  explicit EventClass(EventType type);
  EventClass(EventType type, int val);
  EventClass(EventType type, Cell cell);
  EventClass(EventType type, ra::Target src, ra::Target dest);
  EventClass(ra::Target src, MissionType mission,
             ra::Target target = ra::Target{},
             ra::Target destination = ra::Target{});

  EventClass(ra::Target src, MissionType mission, ra::Target target,
             ra::Target destination, SpeedType speed, int maxspeed);

  EventClass(EventType type, RTTIType object, int id);
  EventClass(EventType type, RTTIType object, Cell cell);
  // A special weapon `id` placed at `where`, a sub-cell coordinate within
  // `cell`.
  EventClass(EventType type, int id, Cell cell, Coordinate where);
  EventClass(AnimType anim, HousesType owner, Coordinate coord);
  EventClass(EventType type, std::span<std::byte> payload);

  // Views the owned payload initialized from a span or an exact-size allocation.
  // Only valid for ADDPLAYER events after the wire pointer has been replaced.
  [[nodiscard]] std::span<std::byte> variable_bytes() const;

  // Gives a received ADDPLAYER event a heap buffer of `size` bytes for its
  // payload and returns it. The event is a trivially copyable record that
  // travels by value through the queues and over the wire, so it cannot hold
  // a smart pointer: the buffer belongs to whichever copy is executed or
  // dropped, and that copy releases it with Free_Variable().
  std::span<std::byte> Allocate_Variable(uint32_t size);
  void Free_Variable();

  // Returns a copy of payload arm T. Type must be one T carries.
  template <class T>
  [[nodiscard]] T data() const {
    DCHECK(T::Carries(Type));
    return data_.Get<T>();
  }
  // Copies arm T out, lets `edit` change it and copies it back; the payload
  // bytes past T keep what they held. Set Type first: it must be one T
  // carries.
  template <class T, std::invocable<T&> Edit>
  void UpdateData(Edit&& edit) {
    DCHECK(T::Carries(Type));
    data_.Update<T>(std::forward<Edit>(edit));
  }
  // The payload read as a FRAMEINFO header, whatever Type says: the first
  // event of every packet is one, and a receiver reads it before it trusts
  // the sender's Type.
  [[nodiscard]] FrameInfoData packet_frame_info() const {
    return data_.Get<FrameInfoData>();
  }
  // The payload as it goes over the wire, for the packet packer.
  [[nodiscard]] std::span<const std::byte> payload_bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return data_.bytes();
  }
  [[nodiscard]] std::span<std::byte> payload_bytes()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return data_.bytes();
  }
  // Zeroes the payload.
  void ClearData() { data_ = {}; }

  // Where the payload starts in the event's bytes: the header before it is
  // Type, then Frame, ID and IsExecuted in one word.
  static constexpr std::size_t kPayloadOffset = 8;

  // Process the event.
  void Execute();

  static base::EnumArray<EventType, unsigned char, static_cast<int>(LAST_EVENT)>
      EventLength;
  static base::EnumArray<EventType, const char*, static_cast<int>(LAST_EVENT)>
      EventNames;

 private:
  base::Payload<OptionsData, SellCellData, TargetData, AnimData, GeneralData,
                MegaMissionData, MegaMissionFormationData, MissionData,
                NavComData, SpecificData, PlaceData, SpecialData, FrameInfoData,
                VariableData, TimingData, ProcessTimeData>
      data_;
};

// Events cross the wire as raw bytes, so their size is part of the protocol.
static_assert(sizeof(EventClass) == 32);
static_assert(alignof(EventClass) == 8);
static_assert(std::is_trivially_copyable_v<EventClass>);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_EVENT_H_
