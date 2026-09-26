#pragma once

// What every script has in common, whatever kind of scene it belongs to: the
// engine creates it by its name, see ScriptRegistry, and may call its Update
// once per frame.
//
// Scripts never derive from this directly. A script of a normal scene is a
// Script and sees the scene with its objects, a script on the tiles of a
// tileset scene is a TileScript and sees its tile and the map. The two kinds
// share nothing else, so each only offers what makes sense where it runs.
class ScriptBase {
public:
    virtual ~ScriptBase() = default;

    ScriptBase(const ScriptBase &) = delete;

    ScriptBase &operator=(const ScriptBase &) = delete;

    // One frame further, dt is the time since the last one in seconds. Which
    // scripts get it from the engine depends on their kind, see Script and
    // TileScript.
    virtual void Update(float dt);

protected:
    ScriptBase() = default;
};
