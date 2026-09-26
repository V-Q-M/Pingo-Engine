#pragma once

#include "TileScript.h"

// Every script of the scene "hub" lives in this namespace, so
// another scene can have a script of the same name
namespace hub {
    // A script on the tiles of the map: the engine calls its Update every
    // frame while the game runs, once for every tile it is assigned to.
    class Portal : public TileScript {
    public:
        void Update(float dt) override;
    };
}
