#pragma once

#include <string>

// The bridge from the tile scripts into a tileset scene, filled by the engine.
//
// Scripts never touch this class: they work with tile and map, see
// scripts/Docs.md. Cells are counted from 0 at the top left, like the lines
// and digits of the map file.
class TileWorld {
public:
    virtual ~TileWorld() = default;

    // Size of the map in cells
    virtual int MapColumns() const = 0;

    virtual int MapRows() const = 0;

    // The digit of a cell as in the map file, 0 outside the map
    virtual int TileNumberAt(int column, int row) const = 0;

    // Changes a cell while the game runs, the file stays as it is. false
    // outside the map or for a digit other than 0 to 9.
    virtual bool SetTileNumberAt(int column, int row, int number) = 0;

    // Name of the tile on a cell as the tile data calls it, e.g. "Grass"
    virtual std::string TileNameAt(int column, int row) const = 0;

    virtual bool IsTileWalkable(int column, int row) const = 0;

    // The cell the player character stands on. While it walks, the cell it
    // came from, until it has arrived on the next one.
    virtual int PlayerColumn() const = 0;

    virtual int PlayerRow() const = 0;

    // The same one frame earlier, to tell when the player arrived or left
    virtual int PreviousPlayerColumn() const = 0;

    virtual int PreviousPlayerRow() const = 0;

    // Does the player walk against this cell in this frame without getting
    // onto it, e.g. against a wall?
    virtual bool IsPlayerPushingAgainst(int column, int row) const = 0;

    // May the player trigger tiles? Not on the cell the scene starts on, and
    // not after a question was cancelled, until it arrived on another cell.
    virtual bool IsPlayerReady() const = 0;

    // false holds the tile events back until the player arrived on another
    // cell, true lets them through again right away
    virtual void SetPlayerReady(bool ready) = 0;

    // Schedules the change into the scene with this id, with confirm only
    // after the player agreed in a window. false if there is no such scene
    // or a question is already open.
    virtual bool SwitchToScene(const std::string &id, bool confirm) = 0;
};
