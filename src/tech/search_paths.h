#ifndef CNC_RED_ALERT_TECH_SEARCH_PATHS_H_
#define CNC_RED_ALERT_TECH_SEARCH_PATHS_H_

// File: the directories the game searches for loose data files, and the
// lookup of a file name through them.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The registry of directories to look in for a data file that is not in the
// current directory. Directories are added from semicolon-separated lists
// such as the -CD command line switch; "?:" stands for the current CD drive.
//
// Example:
//   SearchPaths::Add("C:\\GameData;?:\\");
//   if (auto path = SearchPaths::Resolve("RULES.INI")) {
//     DiskFile file(*path);
//   }
class SearchPaths {
 public:
  SearchPaths() = delete;

  // Appends each directory in the semicolon-separated paths to the search
  // list, normalized to end in a separator. A directory written as "?:" is
  // added, with the drive letter filled in, only when the current CD drive
  // holds a recognized CD (see SetCdDrive). Returns 0 if at least one
  // directory was added and 1 otherwise, which is what callers test.
  static int Add(std::string_view paths);

  // Removes every directory from the search list. Refresh() can bring them
  // back.
  static void Clear();

  // Clears the list and re-adds every directory ever given to Add(), so that
  // "?:" resolves against the current CD drive again after a CD change.
  static void Refresh();

  // Returns true if any directory is on the search list.
  static bool HasAny();

  // Makes drive (0 = A:) the current CD drive, remembering the previous one
  // as the last CD drive.
  static void SetCdDrive(int drive);
  static int current_cd_drive();
  static int last_cd_drive();

  // Returns the path of a loose file called name: name itself if such a file
  // exists, otherwise the first search directory holding one. A name that
  // exists only in lowercase resolves to the lowercase path. Returns nullopt
  // for an empty name or a file found nowhere.
  static std::optional<std::string> Resolve(std::string_view name);

 private:
  // Add() without recording the directories in history_.
  static int Scan(std::string_view paths);

  static std::vector<std::string> directories_;

  // Every list ever passed to Add(), joined by ';', for Refresh().
  static std::string history_;

  static int current_cd_drive_;
  static int last_cd_drive_;
};

#endif  // CNC_RED_ALERT_TECH_SEARCH_PATHS_H_
