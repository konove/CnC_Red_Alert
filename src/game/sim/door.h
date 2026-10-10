// File: Door, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

namespace game {

// The state of a door that opens and closes over a number of animation
// stages, driven by the game's StageClass. Each game has its own StageClass,
// whose saved layout differs, so the door is a template over it; a game names
// its door as `using DoorClass = game::Door<StageClass>;`.
//
// Stage must provide Graphic_Logic(), Fetch_Stage(), Set_Stage(int) and
// Set_Rate(int), and be serializable.
//
// The `stages` a door is opened or closed with is one more than the number of
// frames its animation shows: with 4, Door_Stage() runs 0, 1, 2 while opening
// and stays at 2 once open. The callers' counts allow for it (the war
// factory's door art has one frame fewer than the count it passes).
//
// A door that turns round part-way does not continue from the frame it had
// reached: closing always starts from the fully open frame, and opening from
// the closed one.
//
// Example:
//   DoorClass door;
//   door.Open_Door(/*rate=*/2, /*stages=*/10);
//   while (!door.Is_Door_Open()) door.AI();
template <class Stage>
class Door {
 public:
  Door() = default;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    // A bit-field cannot bind to the archive's reference parameter, so the
    // flag goes through a local.
    bool is_to_redraw = IsToRedraw;
    ar(Control, Stages, State, is_to_redraw);
    if constexpr (Archive::kIsReading) {
      IsToRedraw = is_to_redraw;
    }
  }

  // Returns whether the animation has moved on since the flag was last
  // cleared, so the door's owner needs redrawing.
  [[nodiscard]] bool Time_To_Redraw() const { return IsToRedraw; }
  // Called by the owner once it has drawn the door (or marked itself to be
  // redrawn); const because the draw routines are.
  void Clear_Redraw_Flag() const { IsToRedraw = false; }

  // Advances the animation one game frame; finishes opening or closing, and
  // stops the animation, once the last stage is reached.
  void AI() {
    if (Control.Graphic_Logic()) {
      if (Control.Fetch_Stage() >= Stages) {
        Control.Set_Rate(0);
        switch (State) {
          case kIsOpening:
            State = kIsOpen;
            break;
          case kIsClosing:
            State = kIsClosed;
            break;
          default:
            break;
        }
      }
      IsToRedraw = true;
    }
  }

  // Returns the frame of the door's animation to draw: 0 when closed, up to
  // `stages` - 2 when open (see the class comment).
  [[nodiscard]] int Door_Stage() const {
    switch (State) {
      case kIsClosing:
        return Stages - 1 - Control.Fetch_Stage();
      case kIsClosed:
        return 0;
      case kIsOpening:
        return Control.Fetch_Stage();
      case kIsOpen:
        return Stages - 1;
      default:
        break;
    }
    return 0;
  }

  [[nodiscard]] bool Is_Door_Opening() const { return State == kIsOpening; }
  [[nodiscard]] bool Is_Door_Closing() const { return State == kIsClosing; }

  // Starts opening a closed or closing door, advancing one stage every `rate`
  // game frames; it is open `rate` * (`stages` - 1) frames later. `stages`
  // must be at least 2 and `rate` nonzero, or the door never opens. Returns
  // false, and changes nothing, if it is already open or opening.
  bool Open_Door(int rate, int stages) {
    switch (State) {
      case kIsClosed:
      case kIsClosing:
        State = kIsOpening;
        Stages = static_cast<unsigned char>(stages - 1);
        Control.Set_Stage(0);
        Control.Set_Rate(rate);
        return true;
      default:
        break;
    }
    return false;
  }

  // Starts closing an open or opening door, as Open_Door() opens one. Returns
  // false if it is already closed or closing.
  bool Close_Door(int rate, int stages) {
    switch (State) {
      case kIsOpen:
      case kIsOpening:
        State = kIsClosing;
        Stages = static_cast<unsigned char>(stages - 1);
        Control.Set_Stage(0);
        Control.Set_Rate(rate);
        return true;
      default:
        break;
    }
    return false;
  }

  [[nodiscard]] bool Is_Door_Open() const { return State == kIsOpen; }
  [[nodiscard]] bool Is_Door_Closed() const { return State == kIsClosed; }

 private:
  static constexpr int kIsClosed = 0;   // Door is closed.
  static constexpr int kIsOpening = 1;  // Door is in the process of opening.
  static constexpr int kIsOpen = 2;     // Door is fully open.
  static constexpr int kIsClosing = 3;  // Door is in the process of closing.

  // Counts the stages of the opening or closing in progress, from 0; stopped
  // (rate 0) while the door is open or closed.
  Stage Control;

  // The number of stages of the current door animation, less one: the stage
  // at which the opening or closing is finished.
  unsigned char Stages{0};

  // One of the kIs... values above.
  int State = kIsClosed;

  // Whether the animation moved on, so the object the door belongs to should
  // be redrawn. Drawing clears it from const draw routines, so it is redraw
  // bookkeeping rather than logical state.
  mutable bool IsToRedraw : 1 {false};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
