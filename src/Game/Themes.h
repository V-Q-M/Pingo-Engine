#pragma once

#include "Engine/Theme.h"

// The themes of the game. Currently all scenes share the same one. Every
// scene sets its music itself, see SceneOptions::music.
inline Theme DefaultTheme() {
    Theme theme;

    theme.font = "fonts/game_font.png";

    // Cells of 8 x 12: two pixels of air on top, then the letter, and two
    // more for the tails of g, j, p, q and y. So text sits in the middle of
    // anything that is as high as a cell, see FontRenderer.
    theme.letterWidth = 8;
    theme.letterHeight = 12;

    return theme;
}
