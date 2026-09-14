#pragma once

#include <string>

// A scene of the game as it is listed in the SceneCatalog and shown by the
// engine, e.g. in the scene bar
struct SceneEntry {
    // Id, e.g. "hub". The same one is in the SceneOptions of the scene, and the
    // names of its files are derived from it.
    std::string id;

    // Display name, e.g. "Hub"
    std::string name;

    // Type of the scene, e.g. "tileset", see SceneCatalog::RegisterType
    std::string type;

    // Image from assets/backgrounds behind the whole scene, e.g. "map.png".
    // Empty: the background stays black.
    std::string background;

    // Locked scenes can neither be edited nor deleted
    bool locked = false;

    // The game starts here, see SceneCatalog::EntryPoint
    bool entry = false;
};
