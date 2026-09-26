#pragma once

#include <string>

#include "raylib.h"

// Colors as "#RRGGBB", e.g. in JSON files. Alpha is not written.
std::string ColorToHex(Color color);

// White for anything that is not "#RRGGBB"
Color ColorFromHex(const std::string &text);
