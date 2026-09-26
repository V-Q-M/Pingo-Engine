#include "Api/MapApi.h"

void MapApi::Attach(TileWorld *world) {
    this->world = world;
}

int MapApi::GetColumns() const {
    return world != nullptr ? world->MapColumns() : 0;
}

int MapApi::GetRows() const {
    return world != nullptr ? world->MapRows() : 0;
}

Tile MapApi::GetTile(int column, int row) const {
    return Tile(world, column, row);
}

int MapApi::GetPlayerColumn() const {
    return world != nullptr ? world->PlayerColumn() : 0;
}

int MapApi::GetPlayerRow() const {
    return world != nullptr ? world->PlayerRow() : 0;
}

Tile MapApi::GetPlayerTile() const {
    return GetTile(GetPlayerColumn(), GetPlayerRow());
}

bool MapApi::IsPlayerReady() const {
    return world != nullptr && world->IsPlayerReady();
}

void MapApi::SetPlayerReady(bool ready) {
    if (world != nullptr) {
        world->SetPlayerReady(ready);
    }
}

bool MapApi::SwitchScene(const std::string &id, bool confirm) {
    return world != nullptr && world->SwitchToScene(id, confirm);
}
