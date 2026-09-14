#pragma once

#include <string>
#include <vector>

#include "raylib.h"

// An element of a screen interface, e.g. in the main menu
struct UiElement {
    enum class Type {
        // Only text, e.g. a heading
        Label,

        // Text that reacts to clicks
        Button
    };

    Type type = Type::Label;

    std::string text;

    // Top left corner in viewport coordinates
    Vector2 position{0.0f, 0.0f};

    int scale = 2;

    // Buttons only: what a click triggers, e.g. "scene", "settings" or "quit".
    // The scene defines what the actions mean.
    std::string action;

    // Target of the action, for "scene" the scene
    std::string target;

    // Locked elements can only be moved in the editor
    bool locked = false;

    // Font color, e.g. FontVariant::Red. -1: the color from the theme, the title
    // color for texts and the text color for buttons.
    int variant = -1;

    // For the undo history
    bool operator==(const UiElement &other) const;
};

// Loads and saves an interface as JSON under assets
class UiLayout {
public:
    static constexpr int DEFAULT_SCALE = 2;

    // false if the file exists but cannot be read. It must not be overwritten
    // then. If the file is missing, there are simply no elements.
    static bool Load(const std::string &filename, std::vector<UiElement> &elements);

    // Saves like SaveAsset into both asset folders. Anything else in the file
    // is kept.
    static bool Save(const std::string &filename, const std::vector<UiElement> &elements);
};
