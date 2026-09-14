#pragma once

#include "Engine/Theme.h"

// The themes of the game. Currently all scenes share the same one. Every
// scene sets its music itself, see SceneOptions::music.
inline Theme DefaultTheme() {
    Theme theme;

    theme.font = "fonts/game_font.png";

    return theme;
}
