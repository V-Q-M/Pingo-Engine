#pragma once

#include "raylib.h"

#include "CharacterDefinition.h"
#include "Engine/FontRenderer.h"
#include "Engine/FormWindow.h"

// The advanced settings of a character: size and offset of its shadow and of
// its hitbox, in whole pixels. X and Y move them, negative values go left and
// up. The shadow bounce lets the shadow shrink while the character is up in
// its animation, in the rhythm of the animation itself.
//
// Opens above the edit window. Done hands the values back, Cancel forgets them.
// Both lead back to the edit window.
class AdvancedSettingsWindow {
public:
    enum class Result {
        None,
        Done,
        Cancelled
    };

    void Open(const CharacterBody &body, int viewWidth, int viewHeight, const FontRenderer &font);

    bool IsOpen() const;

    bool IsHovering() const;

    // The mouse in viewport coordinates. On Done and Cancel the window closes.
    Result Update(Vector2 mouse, bool clicked, const FontRenderer &font);

    // The values in the window, also after it closed
    CharacterBody Read() const;

    void Close();

    void Draw(const FontRenderer &font) const;

private:
    // The first field of each box, width, height, X and Y follow in this order
    enum class Field {
        Shadow = 0,
        Hitbox = 4,

        // How much the shadow shrinks while the character is up
        Bounce = 8
    };

    CharacterBox ReadBox(Field first) const;

    FormWindow form;
};
