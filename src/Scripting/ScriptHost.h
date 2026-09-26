#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Api/Script.h"
#include "Api/ScriptWorld.h"
#include "Api/TileScript.h"
#include "Api/TileWorld.h"

// The scripts that run in a scene.
//
// A normal scene has its Main script, which the engine calls once per frame.
// Every other script there is a tool that Main calls itself, so there is
// exactly one place where a frame of the game begins.
//
// A tileset scene has no Main: every tile with a script gets its own
// instance of it, and all of them are called once per frame, in the order
// they were added.
//
// Scripts only run while the game runs: in the Debugging and Release build,
// and in the simulation of the Development build. In the editor itself the
// world stands still and no script is created at all.
class ScriptHost {
public:
    // Creates the Main script of the scene and gives it the world it runs in,
    // see SceneApi. Without a built script, e.g. because the file was only
    // just written, nothing happens later on.
    void Load(const std::string &scene, ScriptWorld &world);

    // Creates one instance of a tile script for the cell at column and row.
    // false if the game does not know the script or it is no TileScript.
    bool AddTileScript(const std::string &scene,
                       const std::string &name,
                       TileWorld &world,
                       int column,
                       int row);

    // Throws all scripts away, e.g. when leaving the scene
    void Clear();

    bool HasMain() const;

    // One frame of the game, dt in seconds: Main first, then the tiles
    void Update(float dt);

private:
    std::unique_ptr<Script> main;

    std::vector<std::unique_ptr<TileScript>> tileScripts;
};
