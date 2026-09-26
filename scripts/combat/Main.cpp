#include "combat/Main.h"

#include "ScriptRegistry.h"

namespace combat {
    void Main::Update(float dt) {
        (void) dt;

        // Your game loop, e.g. Gravity().Run(dt);
    }
}

// Makes the script known to the engine, see ScriptRegistry
PINGO_SCRIPT("combat", "Main", combat::Main)
