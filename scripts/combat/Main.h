#pragma once

#include "Script.h"

namespace combat {
    // The script of the scene: the engine calls its Update every frame
    // while the game runs. Call your own scripts from here.
    class Main : public Script {
    public:
        void Update(float dt) override;
    };
}
