#pragma once

#include <string>

#include "TileWorld.h"

// One cell of the map of a tileset scene. Every tile script has its own
// ready as "tile":
//
//     if (tile.HasPlayerEntered()) {
//         map.SwitchScene("combat");
//     }
//
// Like Object it is only a handle that asks the map again on every call. A
// cell outside the map is empty: Exists() is false, reading gives 0, false or
// an empty text, and setting does nothing.
//
// The player calls (IsPlayerOn, HasPlayerEntered, HasPlayerLeft,
// IsPlayerPushing) only answer while the player is ready, see
// MapApi::IsPlayerReady. That way a cancelled question does not come back in
// the next frame.
//
// Everything a script changes is gone when the scene is left: the map file
// is never written while the game runs.
class Tile {
public:
    // The empty tile, see the class comment
    Tile() = default;

    Tile(TileWorld *world, int column, int row);

    // Does the cell lie on the map?
    bool Exists() const;

    // Where the cell is, counted from 0 at the top left
    int GetColumn() const;

    int GetRow() const;

    // The digit of the cell as in the map file, 0 for an empty cell
    int GetNumber() const;

    // Puts a different tile on the cell, e.g. an open door. 0 to 9, other
    // numbers are ignored.
    void SetNumber(int number);

    // Name of the tile as the tile data calls it, e.g. "Grass"
    std::string GetName() const;

    // Can the player walk onto it?
    bool IsWalkable() const;

    // Does the player stand on this cell? While walking it still counts for
    // the cell it came from, until it has arrived on the next one. false while
    // the player is not ready.
    bool IsPlayerOn() const;

    // Did the player arrive on this cell in this frame? True for exactly one
    // frame per visit. The cell the scene starts on does not count, see
    // MapApi::IsPlayerReady.
    bool HasPlayerEntered() const;

    // Did the player walk off this cell in this frame, i.e. arrive on the
    // next one?
    bool HasPlayerLeft() const;

    // Does the player walk against this cell without getting onto it, e.g.
    // a sign or a locked door? True in every frame the key is held.
    bool IsPlayerPushing() const;

private:
    // On the map and the player ready, see the class comment
    bool CanTrigger() const;

    TileWorld *world = nullptr;

    int column = 0;
    int row = 0;
};
