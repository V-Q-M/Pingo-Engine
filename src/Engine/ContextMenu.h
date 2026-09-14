#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raylib.h"

#include "FontRenderer.h"

// Small window with a list that opens at a position, e.g. after
// a right click.
//
// Works in viewport coordinates and therefore belongs in the UI layer. If it
// would stick out over the edge, it is pushed inwards.
class ContextMenu {
public:
    // No item chosen
    static constexpr int NOTHING = -1;

    void SetTitle(std::string title);

    // If the menu is open, it stays where it is and is only pushed inwards
    // if it now sticks out over the edge
    void SetItems(std::vector<std::string> items);

    // Font variant of an item, e.g. green for a special item.
    // Every item turns yellow under the mouse.
    void SetItemVariant(std::size_t index, int variant);

    // Icon in front of an item, e.g. FontRenderer::ICON_DELETE, 0 for none. If one
    // item has an icon, all texts are indented so they stay aligned.
    void SetItemIcon(std::size_t index, char icon);

    // Disabled items are shown in grey, do not light up and cannot be chosen
    void SetItemEnabled(std::size_t index, bool enabled);

    bool IsItemEnabled(std::size_t index) const;

    bool HasItems() const;

    std::size_t ItemCount() const;

    const std::string &Item(std::size_t index) const;

    // The selectable item under the mouse, otherwise NOTHING
    int ItemAt(Vector2 mousePosition, const FontRenderer &font) const;

    // Was the mouse over an item during the last Update?
    bool IsHovering() const;

    // position is the desired top left corner
    void Open(Vector2 position, int viewWidth, int viewHeight, const FontRenderer &font);

    void Close();

    bool IsOpen() const;

    // The clicked item, otherwise NOTHING. The menu closes after a
    // choice and on a click next to it.
    int Update(Vector2 mousePosition, bool clicked, const FontRenderer &font);

    // The whole window
    Rectangle Bounds(const FontRenderer &font) const;

    // What an item reacts to: the full width, without gaps to the neighboring row
    Rectangle ItemBounds(std::size_t index, const FontRenderer &font) const;

    void Draw(const FontRenderer &font) const;

private:
    Vector2 Size(const FontRenderer &font) const;

    float LineHeight(const FontRenderer &font) const;

    // Space for the icons left of the texts, 0 without icons
    float IconColumn(const FontRenderer &font) const;

    // Pushes the window inwards so it stays fully visible
    void KeepInside(const FontRenderer &font);

    std::string title;

    std::vector<std::string> items;

    // -1 for the normal font
    std::vector<int> variants;

    // 0 for no icon
    std::vector<char> icons;

    std::vector<bool> enabled;

    int viewWidth = 0;
    int viewHeight = 0;

    Vector2 topLeft{0.0f, 0.0f};

    bool open = false;

    int hovered = NOTHING;
};
