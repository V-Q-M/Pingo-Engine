#include "ScriptRegistry.h"

#include "hub/Portal.h"

namespace hub {
    void Portal::Update(float dt) {
        (void) dt;

        if (tile.HasPlayerEntered()) {
             map.SwitchScene("combat");
        }
    }
}

// Makes the script known to the engine, see ScriptRegistry
PINGO_SCRIPT("hub", "Portal", hub::Portal)
