#pragma once

#include "Script.h"

// Every script of the scene "combat_copy" lives in this namespace, so
// another scene can have a script of the same name
namespace combat_copy {
    // The script of the scene: the engine calls its Update every frame
    // while the game runs. Call your own scripts from here.
    class Main : public Script {
    public:
        void Update(float dt) override;
    };
}
