#pragma once

#include <string>

#include "raylib.h"

#include "FontRenderer.h"

// Look and sound of a scene
struct Theme {
    // Font atlas and cell size, see FontRenderer
    std::string font;
    int letterWidth = 8;
    int letterHeight = 8;

    // Overlay and font variants of the menus
    Color menuOverlay{20, 20, 28, 180};
    int textVariant = FontVariant::White;
    int hoverVariant = FontVariant::Yellow;
    int titleVariant = FontVariant::Cyan;
};
