#pragma once

#include <string>

#include "FieldScene.h"
#include "SceneObjects.h"

// A scene with freely placed characters without free movement, e.g. a combat.
//
// The characters are stored in assets/scenes/<id>.json, the music is chosen
// in the scene bar and the engine draws the background, see SceneEntry. All
// scenes share the character types like Pingo, they live under
// assets/characters.
class NormalScene : public FieldScene {
public:
    // id chooses the file, e.g. "combat" for assets/scenes/combat.json
    explicit NormalScene(Engine &engine, const std::string &id = "combat");

    void LeaveFromPauseMenu() override;
};
