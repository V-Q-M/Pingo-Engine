#pragma once

#include "ScriptBase.h"
#include "MapApi.h"
#include "Tile.h"

// Base of every script of a tileset scene.
//
// A tile script sits on the tiles it is assigned to in the editor: right
// click a tile, Scripts. There is no Main: every tile with a script gets its
// own instance of it, and the engine calls the Update of each one once per
// frame while the game runs. A script on three tiles therefore runs three
// times per frame, each time with its own tile.
//
// Like every script it only sees this API, see scripts/Docs.md, does not run
// in the Development mode and never saves what it changes.
class TileScript : public ScriptBase {
protected:
    TileScript() = default;

    // The tile this instance sits on, see Tile
    Tile tile;

    // The map around it and the way to other scenes, see MapApi
    MapApi map;

private:
    friend class ScriptHost;
};
