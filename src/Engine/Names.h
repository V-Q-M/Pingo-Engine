#pragma once

#include <string>

// Helpers for text typed by hand, e.g. names of scenes, types and groups in
// the editor, or commands in the console

// The text without spaces at the edges
std::string TrimSpaces(std::string text);

// The text in lowercase, e.g. to compare without caring about case. Only
// touches A to Z, everything else stays as it is.
std::string ToLower(std::string text);

// Id from a name: lowercase letters, digits and _, e.g. "Dark Forest" becomes
// dark_forest. fallback if nothing is left, e.g. for "!!!".
std::string IdFromName(const std::string &name, const std::string &fallback);
