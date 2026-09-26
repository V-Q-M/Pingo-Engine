#pragma once

#include <string>

#include "Theme.h"

// What the engine turns on for a scene. Everything starts off: a scene only
// turns on what it needs.
struct SceneOptions {
    // Id of the scene, e.g. "hub", as listed in the SceneCatalog. The scene bar
    // in the Development mode uses it to mark the current scene.
    std::string id;

    // ESC opens the pause menu
    bool pauseMenu = false;

    // Label of the last item in the pause menu. The scene decides what it does in
    // Scene::LeaveFromPauseMenu.
    std::string pauseLeaveLabel = "Quit";

    // The movement actions of the input (arrow keys etc.) are active. If this is
    // off, InputMap::MovementDirection always returns (0, 0).
    bool movement = false;

    // In the Development mode the mouse wheel zooms the view, space resets the
    // zoom. Meant for worlds, not for menus.
    bool developmentZoom = false;

    // While playing, a click selects the player character as well. Scenes in
    // which the player is only walking around, e.g. the hub, turn this off.
    // In the Development mode the player stays selectable either way.
    bool selectablePlayer = true;

    // The scene has scripts: the editor offers them in its add menu, and while
    // the game runs the engine calls the Update of its Main script. See
    // ScriptHost.
    bool scripts = false;

    // Music that starts when entering the scene. The music of the previous scene
    // stops, so empty means silence. If the same track is already playing, it
    // continues seamlessly.
    std::string music;

    Theme theme;
};
