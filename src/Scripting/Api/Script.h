#pragma once

#include "ScriptBase.h"
#include "SceneApi.h"

// Base of every script of a normal scene.
//
// Scripts are normal C++ files under scripts/<scene>/, one folder per scene
// and one namespace per folder. They are built as their own target that only
// sees this API: an engine header cannot be included there, so a script can
// only reach the game through what is documented in scripts/Docs.md.
//
// Every normal scene has a script Main, whose Update the engine calls once
// per frame while the game runs. Everything else a script offers are public
// methods that Main, or another script, calls. Only Main gets Update from the
// engine.
//
// Scripts do not run in the Development mode: there the world stands still and
// only the editors work. What a script changes is never saved.
class Script : public ScriptBase {
protected:
    Script() = default;

    // The scene this script runs in, see SceneApi
    SceneApi scene;

private:
    friend class ScriptHost;
};
