// Process-wide access to one instance of a type, installed by its owner.

#ifndef CNC_RED_ALERT_BASE_INSTALLED_H_
#define CNC_RED_ALERT_BASE_INSTALLED_H_

#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/check.h"

namespace base {

// Gives the code that needs a subsystem a way to reach it without the
// subsystem being a global variable. The owner installs its instance with a
// Scope, and everyone else calls Get(). A test installs its own instance the
// same way, instead of defining a global for the code under test to link
// against.
//
// Installed<T> is for the main thread only: installing and reading are not
// synchronized. Get() is a pointer load and a CHECK; a hot loop should take
// the reference once, outside the loop.
//
// Example:
//   class Game {
//     Screen screen_;
//     base::Installed<Screen>::Scope screen_scope_{screen_};
//   };
//
//   Screen& TheScreen() { return base::Installed<Screen>::Get(); }
template <class T>
class Installed {
 public:
  // Installs an instance for the Scope's lifetime and puts back whatever was
  // installed before when it ends. Scopes of one T must end in the reverse
  // order they began; ending one out of order CHECK-fails.
  class Scope {
   public:
    explicit Scope(T& instance ABSL_ATTRIBUTE_LIFETIME_BOUND)
        : instance_(&instance),
          previous_(std::exchange(installed_, &instance)) {}
    ~Scope() {
      CHECK(installed_ == instance_)
          << "Installed<T> scopes ended out of order";
      installed_ = previous_;
    }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;

   private:
    T* instance_;
    T* previous_;  // nullptr if nothing was installed before this scope
  };

  Installed() = delete;

  // Returns the installed instance. CHECK-fails if nothing is installed:
  // before the owner has built it, after it is gone, or in a test that did
  // not install one.
  static T& Get() {
    CHECK(installed_ != nullptr) << "Installed<T>::Get(): nothing is installed";
    return *installed_;
  }

  // Returns whether an instance is installed.
  static bool IsInstalled() { return installed_ != nullptr; }

 private:
  static inline T* installed_ = nullptr;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_INSTALLED_H_
