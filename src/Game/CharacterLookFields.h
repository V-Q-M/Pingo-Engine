#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "CharacterDefinition.h"
#include "Engine/Assets.h"
#include "Engine/FormWindow.h"

// The fields for the look of a character in a FormWindow, e.g. in the windows
// for new and edited objects.
//
// A dropdown chooses a sprite sheet from sprites/characters. With "None" the
// character is a colored square: size and color show up. With a sprite its
// frame width, frame height and number of frames show up instead. Choosing a
// sprite fills in frames that fit it.
//
// Below that every effect has a checkbox: a character that shows the effect
// also gets a field for the row of its sheet that animates during the effect,
// see ObjectEffects.
class CharacterLookFields {
public:
    // Colors for squares, from the game's palette
    static const std::vector<Color> &Colors();

    // Appends the fields to the form, filled with look
    void Add(FormWindow &form, const CharacterLook &look);

    // Call after every FormWindow::Update: shows the fields of the chosen sprite
    // and fills in its frames when it was just chosen
    void Follow(FormWindow &form, Assets &assets);

    // The look the fields describe
    CharacterLook Read(const FormWindow &form) const;

private:
    // Same order as in the form, the effects follow
    enum class Field {
        Sprite,
        Size,
        Color,
        FrameWidth,
        FrameHeight,
        Frames
    };

    std::size_t Index(Field field) const;

    // Checkbox of the effect, its row field sits right after it
    std::size_t EffectIndex(std::size_t effect) const;

    void ShowFieldsFor(FormWindow &form) const;

    // The first field in the form
    std::size_t first = 0;

    // The checkbox of the first effect, every effect takes two fields
    std::size_t firstEffect = 0;

    // The sprite sheets in the dropdown, without "None" in front
    std::vector<std::string> sprites;

    // The dropdown option the fields currently belong to
    std::size_t shownSprite = 0;
};
