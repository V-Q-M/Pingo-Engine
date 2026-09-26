#pragma once

#include <string>

#include "Tile.h"

// The map of the tileset scene a tile script runs in. Every tile script has
// it ready as "map":
//
//     if (tile.HasPlayerEntered()) {
//         map.SwitchScene("combat");
//     }
//
//     map.GetTile(3, 2).SetNumber(6);
//
// Cells are counted from 0 at the top left, like the lines and digits of the
// map file.
class MapApi {
public:
    // Size of the map in cells
    int GetColumns() const;

    int GetRows() const;

    // Any cell of the map, an empty Tile outside of it
    Tile GetTile(int column, int row) const;

    // The cell the player stands on, see Tile::IsPlayerOn
    int GetPlayerColumn() const;

    int GetPlayerRow() const;

    Tile GetPlayerTile() const;

    // May the player trigger tiles? While not, the player calls of Tile all
    // answer false. The player is not ready on the cell the scene starts on
    // and after cancelling the question of SwitchScene, until it arrived on
    // another cell.
    bool IsPlayerReady() const;

    // false holds the tile events back until the player walked onto another
    // cell, e.g. after a sign was read. true lets them through right away.
    void SetPlayerReady(bool ready);

    // Leaves this scene for the one with this id, as it stands in
    // assets/scenes/scenes.json, e.g. "combat".
    //
    // With confirm, the default, the game first asks "Travel to <name>?"
    // with Confirm and Cancel and stands still meanwhile. Cancel makes the
    // player not ready, see IsPlayerReady, so the question does not come
    // back right away. Without confirm the change happens after this frame.
    //
    // false if there is no scene with this id, or a question is open already.
    bool SwitchScene(const std::string &id, bool confirm = true);

private:
    friend class ScriptHost;

    // The engine hands the map to the script, see ScriptHost
    void Attach(TileWorld *world);

    TileWorld *world = nullptr;
};
