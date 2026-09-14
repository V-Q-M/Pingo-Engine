#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "Assets.h"

// Describes the cells a TileMap is made of: an atlas with square images and,
// for every digit, whether the cell can be entered.
//
// Digit 1 is the first image in the atlas, digit 2 the second and so on, row
// by row from the top left. 0 is always empty.
class TileSet {
public:
    static constexpr int MAX_TILE = 9;

    // Placeholder without an atlas: every cell except 0 is walkable and is drawn
    // as a yellow and blue checkerboard
    explicit TileSet(int tileSize = 32);

    // If the file is missing or broken, the placeholder is used
    static TileSet Load(Assets &assets, const std::string &filename);

    int TileSize() const;

    // Does the TileSet know this digit?
    bool IsDefined(int tile) const;

    bool IsWalkable(int tile) const;

    // Name from the description, e.g. "Grass". Empty if there is none.
    const std::string &Name(int tile) const;

    // All digits that can be set: 0 for empty and the described cells. Without a
    // description all from 0 to MAX_TILE.
    std::vector<int> AvailableTiles() const;

    // Draws a cell with its top left corner at x, y
    void Draw(int tile, int x, int y) const;

private:
    struct TileInfo {
        bool defined = false;
        bool walkable = false;

        std::string name;
    };

    void DrawPlaceholder(int x, int y) const;

    void DrawMissing(int x, int y) const;

    Texture2D *texture = nullptr;

    int tileSize;

    // How many images the atlas has side by side and on top of each other
    int columns = 0;
    int rows = 0;

    // Was the TileSet described by a file? Without a description every cell
    // counts as walkable.
    bool described = false;

    // The index is the digit, 0 stays unused
    std::vector<TileInfo> tiles;
};
