#pragma once

#include "ConsoleCommands.h"
#include "SceneOptions.h"

class Engine;

// A section of the game, e.g. main menu, hub or combat.
//
// A scene starts as an empty canvas: the engine provides assets, renderer,
// input, font and settings, and the scene only turns on what it needs via
// its SceneOptions. All methods are optional.
//
// Lifecycle: constructor (loading), Enter, then Update, Draw and DrawUI every
// frame, finally Exit and destructor. The engine applies the SceneOptions
// between constructor and Enter.
class Scene {
public:
    Scene(Engine &engine, SceneOptions options);

    virtual ~Scene() = default;

    Scene(const Scene &) = delete;

    Scene &operator=(const Scene &) = delete;

    const SceneOptions &Options() const;

    virtual void Enter();

    virtual void Exit();

    // Not called while a menu of the engine is open, and not in the
    // Development mode
    virtual void Update(float dt);

    // Runs instead of Update in the Development mode. The world stands still
    // then, only developer tools like the tile editor belong here.
    virtual void UpdateDevelopment(float dt);

    // Does the scene currently need the arrow keys itself, e.g. to move a
    // character? Otherwise they control the free camera in the Development mode.
    // WASD always stays with the camera.
    virtual bool UsesArrowKeys() const;

    // Is the scene typing text right now, e.g. into a form? Then the free camera
    // stands still, not even WASD moves it.
    virtual bool CapturesKeyboard() const;

    // Extent of the world, so the free camera in the Development mode knows how
    // far it may move. Width 0: as large as the viewport.
    virtual Rectangle WorldArea() const;

    // World layer: camera active, submit and draw here
    virtual void Draw();

    // Screen layer without camera, lies below the engine's menus
    virtual void DrawUI();

    // Above everything, in the pixels of the window instead of the grid of
    // the viewport: for windows that want more than the coarse grid, e.g. the
    // code editor. Default: nothing, and then DrawsScreenLayer says so.
    virtual void DrawScreen();

    virtual bool DrawsScreenLayer() const;

    // ESC was pressed. true if the scene needed the key itself, e.g. to close
    // its own window. Then no pause menu opens. Default: false.
    virtual bool OnEscape();

    // The last item in the pause menu was chosen, see
    // SceneOptions::pauseLeaveLabel. Default: quit the game.
    virtual void LeaveFromPauseMenu();

    // Adds the console commands of this scene, e.g. "damage". The engine asks
    // once the scene has started and throws them away with the scene. How to
    // write one is explained in ConsoleCommands. Default: none.
    // Only reachable in the Debugging and the Development build.
    virtual void AddCommands(ConsoleCommands &commands);

protected:
    Engine &engine;

private:
    SceneOptions options;
};
