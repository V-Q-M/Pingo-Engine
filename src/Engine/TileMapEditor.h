#pragma once

#include <vector>

#include "raylib.h"

#include "ContextMenu.h"
#include "FontRenderer.h"
#include "TileMap.h"

// Developer tool for a TileMap.
//
// Shows its digit on every cell. A click selects a cell, a digit key sets it
// and saves the map right away. A right click also selects the cell and opens
// a menu with all tiles of the TileSet, a click on one sets it. The selection
// stays afterwards, a click next to the map clears it.
//
// It saves to the copy in the build directory, which the game reads from,
// and in the Development build also to the asset folder of the sources. Only
// then does the change survive the next build.
class TileMapEditor {
public:
    struct Input {
        // The mouse in world coordinates, for the cells
        Vector2 mouseWorld{0.0f, 0.0f};

        // The same mouse in viewport coordinates, for the menu
        Vector2 mouseViewport{0.0f, 0.0f};

        bool leftClicked = false;
        bool rightClicked = false;

        int viewWidth = 0;
        int viewHeight = 0;
    };

    explicit TileMapEditor(TileMap &map);

    // Also reads the digits typed in this frame. font is needed for the size of
    // the menu.
    void Update(const Input &input, const FontRenderer &font);

    // Sets the selected cell and saves. false if nothing is selected, the cell
    // already has this digit or saving fails.
    bool ApplyDigit(int tile);

    bool HasSelection() const;

    bool IsMenuOpen() const;

    void CloseMenu();

    // Is the mouse over an item of the menu?
    bool IsHovering() const;

    const ContextMenu &TileMenu() const;

    // The digits of the menu items, in their order
    const std::vector<int> &MenuTiles() const;

    // Belongs in the overlay layer: world coordinates, above the characters
    void Draw(const FontRenderer &font) const;

    // Belongs in the screen layer: the menu above everything
    void DrawMenu(const FontRenderer &font) const;

private:
    // Like the add menu, the menu opens slightly offset next to the mouse
    static constexpr float MENU_OFFSET = 5.0f;

    bool Save() const;

    void OpenMenu(const Input &input, const FontRenderer &font);

    TileMap &map;

    int hoveredColumn = -1;
    int hoveredRow = -1;

    int selectedColumn = -1;
    int selectedRow = -1;

    ContextMenu menu;

    std::vector<int> menuTiles;
};
