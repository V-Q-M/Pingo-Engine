#include "ScriptRegistry.h"

#include "combat_copy/Main.h"

namespace combat_copy {
    void Main::Update(float dt) {
        (void) dt;

        // Your game loop, e.g. Gravity().Run(dt);
    }
}

// Makes the script known to the engine, see ScriptRegistry
PINGO_SCRIPT("combat_copy", "Main", combat_copy::Main)
