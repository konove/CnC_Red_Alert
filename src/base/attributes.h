// File: compiler attributes with one portable spelling.

#ifndef CNC_RED_ALERT_BASE_ATTRIBUTES_H_
#define CNC_RED_ALERT_BASE_ATTRIBUTES_H_

// Marks an enum whose values are bit flags combined with |, & and ~, so that
// clang's enum-cast range analysis accepts any combination of its enumerators
// instead of only the named ones. GCC has no equivalent and rejects the
// attribute under -Werror=attributes, so the macro is empty there.
//
// Example:
//   enum CNC_FLAG_ENUM TextPrintType { TPF_8POINT = 1, TPF_CENTER = 2 };
#if __has_cpp_attribute(clang::flag_enum)
#define CNC_FLAG_ENUM [[clang::flag_enum]]
#else
#define CNC_FLAG_ENUM
#endif

#endif  // CNC_RED_ALERT_BASE_ATTRIBUTES_H_
