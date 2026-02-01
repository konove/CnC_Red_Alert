#ifndef CNC_RED_ALERT_TECH_NOINIT_H_
#define CNC_RED_ALERT_TECH_NOINIT_H_

// Tag type for constructors that skip member initialization.
//
// Used to load serialized data directly into objects with virtual functions.
// The constructor only initializes the vtable pointer, skipping all member
// initialization. After loading raw data, use placement new to properly
// reinitialize the object.
//
// Example:
//   MyClass(NoInitClass const&) {}  // No-init constructor
//   MyClass* obj = new (buffer) MyClass(NoInitClass{});  // Load then reinit
struct NoInitClass {
  void operator()() const {}
};

#endif  // CNC_RED_ALERT_TECH_NOINIT_H_
