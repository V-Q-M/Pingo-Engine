#pragma once

#include "raylib.h"

#include "SpriteInstance.h"
#include "TileMap.h"

// Moves a character cell by cell across a TileMap.
//
// A step always runs through to the next cell. If the direction is still
// held, the next step follows without stopping. There is no diagonal
// movement on the grid.
class GridMover {
public:
    GridMover(const TileMap &map, int column, int row);

    // direction e.g. from InputMap::MovementDirection, speed in pixels per second
    void Update(SpriteInstance &instance, Vector2 direction, float speed, float dt);

    // The cell the character stands on or has just started walking from
    int Column() const;

    int Row() const;

    bool IsMoving() const;

private:
    // Starts a step if the target cell is walkable
    bool TryStart(Vector2 direction);

    const TileMap &map;

    int column;
    int row;

    int targetColumn;
    int targetRow;

    // How far the current step is, 0.0 to 1.0
    float progress = 0.0f;

    bool moving = false;
};
