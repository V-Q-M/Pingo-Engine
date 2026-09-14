#include "GridMover.h"

GridMover::GridMover(const TileMap &map, int column, int row)
    : map(map),
      column(column),
      row(row),
      targetColumn(column),
      targetRow(row) {
}

int GridMover::Column() const {
    return column;
}

int GridMover::Row() const {
    return row;
}

bool GridMover::IsMoving() const {
    return moving;
}

bool GridMover::TryStart(Vector2 direction) {
    int stepX = direction.x > 0.0f ? 1 : (direction.x < 0.0f ? -1 : 0);
    int stepY = direction.y > 0.0f ? 1 : (direction.y < 0.0f ? -1 : 0);

    // If two directions are pressed, the horizontal one wins. If it is blocked,
    // the vertical one is tried: that way you keep moving along a wall instead
    // of getting stuck.
    if (stepX != 0 && map.IsWalkable(column + stepX, row)) {
        targetColumn = column + stepX;
        targetRow = row;
    } else if (stepY != 0 && map.IsWalkable(column, row + stepY)) {
        targetColumn = column;
        targetRow = row + stepY;
    } else {
        return false;
    }

    moving = true;

    return true;
}

void GridMover::Update(SpriteInstance &instance, Vector2 direction, float speed, float dt) {
    if (!moving) {
        progress = 0.0f;
        TryStart(direction);
    }

    if (moving) {
        progress += speed * dt / static_cast<float>(map.TileSize());

        // Arrived. If the direction is still held, the step into the next cell
        // starts right away and the leftover distance is carried over. Otherwise
        // the character would stand still for one frame on every cell.
        while (moving && progress >= 1.0f) {
            column = targetColumn;
            row = targetRow;

            moving = false;
            progress -= 1.0f;

            if (!TryStart(direction)) {
                progress = 0.0f;
            }
        }
    }

    Vector2 from = map.CellFeet(column, row);

    if (!moving) {
        instance.SetPosition(from);
        return;
    }

    Vector2 to = map.CellFeet(targetColumn, targetRow);

    instance.SetPosition({
        from.x + (to.x - from.x) * progress,
        from.y + (to.y - from.y) * progress
    });
}
