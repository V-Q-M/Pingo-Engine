#include "Api/Tile.h"

Tile::Tile(TileWorld *world, int column, int row)
    : world(world),
      column(column),
      row(row) {
}

bool Tile::Exists() const {
    return world != nullptr && column >= 0 && row >= 0 &&
           column < world->MapColumns() && row < world->MapRows();
}

int Tile::GetColumn() const {
    return column;
}

int Tile::GetRow() const {
    return row;
}

int Tile::GetNumber() const {
    return Exists() ? world->TileNumberAt(column, row) : 0;
}

void Tile::SetNumber(int number) {
    if (Exists()) {
        world->SetTileNumberAt(column, row, number);
    }
}

std::string Tile::GetName() const {
    return Exists() ? world->TileNameAt(column, row) : "";
}

bool Tile::IsWalkable() const {
    return Exists() && world->IsTileWalkable(column, row);
}

bool Tile::CanTrigger() const {
    return Exists() && world->IsPlayerReady();
}

bool Tile::IsPlayerOn() const {
    return CanTrigger() && world->PlayerColumn() == column && world->PlayerRow() == row;
}

bool Tile::HasPlayerEntered() const {
    bool before = world != nullptr && world->PreviousPlayerColumn() == column && world->PreviousPlayerRow() == row;

    return IsPlayerOn() && !before;
}

bool Tile::HasPlayerLeft() const {
    bool before = world != nullptr && world->PreviousPlayerColumn() == column && world->PreviousPlayerRow() == row;
    bool now = world != nullptr && world->PlayerColumn() == column && world->PlayerRow() == row;

    return CanTrigger() && before && !now;
}

bool Tile::IsPlayerPushing() const {
    return CanTrigger() && world->IsPlayerPushingAgainst(column, row);
}
