/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Fall Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include "./CrawlFileTree.hpp"
#include <algorithm>
#include <cctype>
#include "./FileReader.hpp"
#include "./HttpUtils.hpp"

using std::nullopt;
using std::optional;
using std::string;

namespace searchserver {

//////////////////////////////////////////////////////////////////////////////
// Internal helper functions and constants
//////////////////////////////////////////////////////////////////////////////

static bool handle_dir(const string& dir_path, WordIndex& index);

// Read and parse the specified file, then inject it into the MemIndex.
static void handle_file(const string& fpath, WordIndex& index);

//////////////////////////////////////////////////////////////////////////////
// Externally-exported functions
//////////////////////////////////////////////////////////////////////////////

optional<WordIndex> crawl_filetree(const string& root_dir) {
  // TODO
  // you probably want to use the helper functions
  // -- implementation below --
  if (!readdir(root_dir)) {
    return nullopt;
  }
  WordIndex index;
  if (!handle_dir(root_dir, index)) {
    return nullopt;
  }
  return index;
}

//////////////////////////////////////////////////////////////////////////////
// Internal helper functions
//////////////////////////////////////////////////////////////////////////////

static bool handle_dir(const string& dir_path, WordIndex& index) {
  // Recursively descend into the passed-in directory, looking for files and
  // subdirectories.  Any encountered files are processed via handle_file(); any
  // subdirectories are recusively handled by handle_dir().

  // TODO
  // you may find the readdir function in HttpUtils helpful
  auto maybe_entries = readdir(dir_path);
  if (!maybe_entries) {
    return false;
  }
  for (auto& e : *maybe_entries) {
    if (e.name == "." || e.name == "..") {
      continue;
    }
    string full = dir_path + "/" + e.name;
    if (e.is_dir) {
      if (!handle_dir(full, index)) {
        return false;
      }
    } else {
      handle_file(full, index);
    }
  }
  return true;
}

static void handle_file(const string& fpath, WordIndex& index) {
  // TODO: implement

  // Read the contents of the specified file into a string
  auto maybe_text = read_file(fpath);
  if (!maybe_text) {
    return;
  }
  string content = *maybe_text;

  // Search the string for all tokens, by splitting on " \r\t\v\n,.:;?!"
  // A delimiter marks the end of a token and is not considered a valid part of
  // the token
  auto tokens = split(content, " \r\t\v\n,.:;?!");

  // Record each non empty token as a word into the Wordindex specified by index
  // Your implementation should also be case in-sensitive and record every word
  // in all lower-case
  for (auto& w : tokens) {
    if (w.empty()) {
      continue;
    }
    std::transform(w.begin(), w.end(), w.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    index.record(w, fpath);
  }
}

}  // namespace searchserver
