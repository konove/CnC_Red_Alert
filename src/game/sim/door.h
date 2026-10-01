// File: DoorClass, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

namespace game {

// The state of a door that opens and closes over a number of animation
// stages, driven by the game's StageClass. Each game has its own StageClass,
// whose saved layout differs, so the door is a template over it; a game names
// its door as `using DoorClass = game::DoorClass<StageClass>;`.
//
// Stage must provide Graphic_Logic(), Fetch_Stage(), Set_Stage(int) and
// Set_Rate(int), and be serializable.
//
// Example:
//   DoorClass door;
//   door.Open_Door(/*rate=*/2, /*stages=*/10);
//   while (!door.Is_Door_Open()) door.AI();
template <class Stage>
class DoorClass {
 public:
  DoorClass() = default;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_to_redraw = IsToRedraw;
    ar(Control, Stages, State, is_to_redraw);
    if constexpr (Archive::kIsReading) {
      IsToRedraw = is_to_redraw;
    }
  }

  [[nodiscard]] bool Time_To_Redraw() const { return IsToRedraw; }
  void Clear_Redraw_Flag() const { IsToRedraw = false; }

  // Advances the animation one game frame; finishes opening or closing once
  // the last stage is reached.
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

  // Returns the stage of the door's animation, 0 (closed) to the number of
  // stages less one (open).
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

  // Starts opening a closed or closing door over `stages` stages, advancing
  // one stage every `rate` frames. Returns false if it is already open or
  // opening.
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

  // Starts closing an open or opening door, as Open_Door() opens one.
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

  Stage Control;  // The animation control handler.

  // The number of stages of the current door animation, less one.
  unsigned char Stages{0};

  int State = kIsClosed;

  // Whether the animation moved on, so the object the door belongs to should
  // be redrawn. Drawing clears it from const draw routines, so it is redraw
  // bookkeeping rather than logical state.
  mutable bool IsToRedraw : 1 {false};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
